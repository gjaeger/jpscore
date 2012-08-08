/**
 * @file LCGrid.h
 * @author   Ulrich Kemloh <kemlohulrich@gmail.com>
 * @version not versioned
 * Copyright (C) <2009-2010>
 *
 * @section LICENSE
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * \brief This class Implements the Linked-Cells algorithm.

 * @section DESCRIPTION
 *   This class implements the Linked-Cells algorithm
 *   \ref{cacs.usc.edu/education/cs596/01-1LinkedListCell.pdf}
 *   A grid is laid on the complete geometry and the pedestrians are assigned the cells
 *   at each simulation step. Only pedestrians in the neighbouring cells are involved
 *   in the force computations.
 *
 *   The class is static as only one instance is needed per simulation round.
 *   This solution is fine for parallelisation as well, at least for OpenMP.
 *
 *
 *  Created on: Nov 16, 2010
 *
 */

#include"LCGrid.h"


LCGrid::LCGrid(double boundaries[4], double cellsize, int nPeds){

	pGrid_xmin=boundaries[0];
	pGrid_xmax=boundaries[1];
	pGrid_ymin=boundaries[2];
	pGrid_ymax=boundaries[3];
	pCellSize=cellsize;
	pNpeds=nPeds;
	// only pedestrians within that pMaxEffectivDist radius will be returned

	pMaxEffectivDist=1.0; //	pMaxEffectivDist=2.2;


	// add 1 to ensure that the whole area is covered by cells if not divisable without remainder
	pGridSizeX = (int) ((pGrid_xmax - pGrid_xmin) / pCellSize) + 1 + 2; // 1 dummy cell on each side
	pGridSizeY = (int) ((pGrid_ymax - pGrid_ymin) / pCellSize) + 1 + 2; // 1 dummy cell on each side

	// allocate memory for cells (2D-array) and initialize
	pCellHead= new int *[pGridSizeY];

	for (int i = 0; i < pGridSizeY; ++i) {
		pCellHead[i]  = new int[pGridSizeX]; // nx columns

		for (int j = 0; j < pGridSizeX; ++j) {
			pCellHead[i][j] = LIST_EMPTY;
		}
	}

	// creating and resetting the pedestrians list
	pList = new int[nPeds];
	for(int i=0;i<nPeds;i++) pList[i]=0;

	//allocating the place for the peds copy
	pLocalPedsCopy=new Pedestrian*[nPeds];
	for(int i=0;i<nPeds;i++) pLocalPedsCopy[i]=NULL;

}

LCGrid::~LCGrid(){
	for(int i=0;i<pNpeds;i++) {
		if(!pLocalPedsCopy[i])
			delete pLocalPedsCopy[i];
	}
	delete [] pList;
	delete [] pLocalPedsCopy;
	for (int i = 0; i < pGridSizeY; ++i)  delete[] pCellHead[i];
	delete[] pCellHead;
}

void LCGrid::ShallowCopy(const vector<Pedestrian*>& peds){

	for(unsigned int p=0;p<peds.size();p++){
		int id= peds[p]->GetPedIndex()-1;
		pLocalPedsCopy[id]=peds[p];
	}
}

void LCGrid::Update(const vector<Pedestrian*>& peds){
	int nSize=peds.size();

	ClearGrid();

	for (int p = 0; p < nSize; p++) {
		Pedestrian* ped = peds[p];
		int id=ped->GetPedIndex()-1;
		// determine the cell coordinates of pedestrian i
		int ix = (int) ((ped->GetPos().GetX() - pGrid_xmin) / pCellSize) + 1; // +1 because of dummy cells
		int iy = (int) ((ped->GetPos().GetY() - pGrid_ymin) / pCellSize) + 1;
		//printf("[%f, %f]  ",ped->GetPos().GetX(), ped->GetPos().GetY());
		// create lists
		//printf("[%d=%d] ",ped->GetPedIndex(),id);
		pList[id] = pCellHead[iy][ix];
		pCellHead[iy][ix] = id;

		pLocalPedsCopy[id]=ped;
	}
}

// I hope you had called Clear() first
void LCGrid::Update(Pedestrian* ped){

	int id=ped->GetPedIndex()-1;
	// determine the cell coordinates of pedestrian i
	int ix = (int) ((ped->GetPos().GetX() - pGrid_xmin) / pCellSize) + 1; // +1 because of dummy cells
	int iy = (int) ((ped->GetPos().GetY() - pGrid_ymin) / pCellSize) + 1;

	// update the list previously created
	pList[id] = pCellHead[iy][ix];
	pCellHead[iy][ix] = id;

	// this is probably a pedestrian coming from the mpi routine, so made a copy
	pLocalPedsCopy[id]=ped;
}

void LCGrid::ClearGrid(){
	// start by resetting the current list
	for (int i = 0; i < pGridSizeY; ++i) {
		for (int j = 0; j < pGridSizeX; ++j) {
			pCellHead[i][j] = LIST_EMPTY;
		}
	}

	//int len=sizeof ( pList ) / sizeof ( *pList );
	//cout<<" size: "<<len<<endl;
	for(int i=0;i<pNpeds;i++) pList[i]=LIST_EMPTY;

}

void LCGrid::GetNeighbourhood(const Pedestrian* ped, Pedestrian** neighbourhood, int* nSize){

	double xPed=ped->GetPos().GetX();
	double yPed=ped->GetPos().GetY();


	int l = (int) ((xPed - pGrid_xmin) / pCellSize) + 1; // +1 because of dummy cells
	int k = (int) ((yPed - pGrid_ymin) / pCellSize) + 1;

	//-1 to get  correct mapping in the array local
	int myID=ped->GetPedIndex()-1;

	*nSize=0;
	// all neighbor cells
	for (int i = l - 1; i <= l + 1; ++i) {
		for (int j = k - 1; j <= k + 1; ++j) {
			//printf(" i=%d j=%d k=%d l=%d\n",i,j,nx,ny);
			int p = pCellHead[j][i];
			// all peds in one cell
			while (p != LIST_EMPTY) {
				double x=pLocalPedsCopy[p]->GetPos().GetX();
				double y=pLocalPedsCopy[p]->GetPos().GetY();
				double dist=((x-xPed)*(x-xPed) + (y-yPed)*(y-yPed));
				if((dist<pMaxEffectivDist*pMaxEffectivDist) && (p!=myID)){
					neighbourhood[(*nSize)++]=pLocalPedsCopy[p];
					if(*nSize>299) return;
				}
				// next ped
				p = pList[p];
			}
		}
	}
}

void LCGrid::GetNeighbourhood(const Pedestrian* ped, vector<Pedestrian*>& neighbourhood){

	double xPed=ped->GetPos().GetX();
	double yPed=ped->GetPos().GetY();

	int l = (int) ((xPed - pGrid_xmin) / pCellSize) + 1; // +1 because of dummy cells
	int k = (int) ((yPed - pGrid_ymin) / pCellSize) + 1;

	//-1 to get  correct mapping in the array local
	int myID=ped->GetPedIndex()-1;

	// all neighbor cells
	for (int i = l - 1; i <= l + 1; ++i) {
		for (int j = k - 1; j <= k + 1; ++j) {
			//printf(" i=%d j=%d k=%d l=%d\n",i,j,nx,ny);
			int p = pCellHead[j][i];
			// all peds in one cell
			while (p != LIST_EMPTY) {
				double x=pLocalPedsCopy[p]->GetPos().GetX();
				double y=pLocalPedsCopy[p]->GetPos().GetY();
				double dist=((x-xPed)*(x-xPed) + (y-yPed)*(y-yPed));
				if((dist<pMaxEffectivDist*pMaxEffectivDist) && (p!=myID)){
					neighbourhood.push_back(pLocalPedsCopy[p]);
				}
				// next ped
				p = pList[p];
			}
		}
	}
}
void LCGrid::getNeighbourhood(const Point& pt, vector<Pedestrian*>& neighbourhood){
	double xPed=pt.GetX();
	double yPed=pt.GetY();


	int l = (int) ((xPed - pGrid_xmin) / pCellSize) + 1; // +1 because of dummy cells
	int k = (int) ((yPed - pGrid_ymin) / pCellSize) + 1;

	// all neighbor cells
	for (int i = l - 1; i <= l + 1; ++i) {
		for (int j = k - 1; j <= k + 1; ++j) {
			//printf(" i=%d j=%d k=%d l=%d\n",i,j,nx,ny);
			int p = pCellHead[j][i];
			// all peds in one cell
			while (p != LIST_EMPTY) {
				double x=pLocalPedsCopy[p]->GetPos().GetX();
				double y=pLocalPedsCopy[p]->GetPos().GetY();
				double dist=((x-xPed)*(x-xPed) + (y-yPed)*(y-yPed));
				if((dist<pMaxEffectivDist*pMaxEffectivDist)){
					neighbourhood.push_back(pLocalPedsCopy[p]);
				}
				// next ped
				p = pList[p];
			}
		}
	}
}


void LCGrid::Dump(){

	for(int l =1;l<pGridSizeY-1;l++){
		for(int k=1;k<pGridSizeX-1;k++){

			int	ped = pCellHead[l][k];

			if(ped==LIST_EMPTY) continue;

			printf("Cell[%d][%d] = { ",l,k);
			//getc(stdin);
			//while (ped != EMPTY) {
			//printf("%d, ",ped+1);

			// all neighbor cells
			for (int i = l - 1; i <= l + 1; ++i) {
				for (int j = k - 1; j <= k + 1; ++j) {
					// dummy cells will be empty
					int p =  pCellHead[i][j];
					// all peds in one cell
					while (p != LIST_EMPTY) {
						printf("%d, ",p+1);
						// next ped
						p = pList[p];
					}
				}
				//}
				// next ped
				//ped = list[ped];
			}
			printf("}\n");
		}
	}
}

void LCGrid::dumpCellsOnly(){

	for(int l =1;l<pGridSizeY-1;l++){
		for(int k=1;k<pGridSizeX-1;k++){

			int	ped = pCellHead[l][k];

			if(ped==LIST_EMPTY) continue;

			printf("Cell[%d][%d] = { ",l,k);
			//getc(stdin);
			//while (ped != EMPTY) {
			//printf("%d, ",ped+1);

			// all neighbor cells
			// dummy cells will be empty
			int p =  pCellHead[l][k];
			// all peds in one cell
			while (p != LIST_EMPTY) {
				printf("%d, ",p+1);
				// next ped
				p = pList[p];
			}
			printf("}\n");
		}
	}
}
