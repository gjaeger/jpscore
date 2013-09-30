/*
 * MeshRouter.cpp
 *
 *  Created on: 21.08.2013
 *      Author: dominik
 */

#include <iomanip>

#include "MeshRouter.h"
#include "../tinyxml/tinyxml.h"
#include "../geometry/Building.h"
#include "../pedestrian/Pedestrian.h"

using namespace std;

MeshRouter::MeshRouter() {
	_building=NULL;
	_meshdata=NULL;
}

MeshRouter::~MeshRouter() {
	delete _meshdata;
}

// Debug
void astar_print(bool* closedlist,bool* inopenlist,int* predlist,
		unsigned int c_totalcount,int act_id,vector<pair<double,MeshCell*> >openlist){
	cout<<"----------------------------------"<<endl;
	cout<<"act_id: "<<act_id<<endl;
	cout<<"Closed-List"<<endl;
	for(unsigned int i=0;i<c_totalcount;i++)
		cout<<(closedlist[i]?"1 ":"0 ");
	cout<<endl;
	cout<<"Inopen-List"<<endl;
	for(unsigned int i=0;i<c_totalcount;i++)
		cout<<(inopenlist[i]?"1 ":"0 ");
	cout<<endl;
	cout<<"Predecessor-List"<<endl;
	for(unsigned int i=0;i<c_totalcount;i++)
		(predlist[i]!=-1?cout<<predlist[i]<<" ":cout<<"* ");
	cout<<endl;
	cout<<"Openlist"<<endl;
	for(unsigned int i=0;i<openlist.size();i++)
		cout<<openlist.at(i).second->GetID()<<"(f="<<openlist.at(i).first<<") ";
	cout<<endl;
	cout<<"----------------------------------"<<endl;
}


void print_path(vector<MeshEdge*>edge_path){
	cout<<"path from start to goal"<<endl;
	for(unsigned int i=0;i<edge_path.size();i++){
		cout<<edge_path.at(i)->toString()<<endl;
	}
}

/* Helper for Funnel
     \   2 /
       \   /
    3   X apex  1
      /    \
     /   0  \
 right   left
 */
int TestinFunnel(Point apex, Point left,Point right,Point test){

	bool r_test=Line(apex,right).IsLeft(test);
	bool l_test=Line(left,apex).IsLeft(test);

	if (r_test){
		if(l_test)
			return 0;
		else
			return 1;
	}else{
		if(l_test)
			return 3;
		else
			return 2;
	}
}

NavLine MeshRouter::Funnel(Point& start,Point& goal,vector<MeshEdge*> edge_path)const{

	if(edge_path.empty()){
		// Start and End Point in same Cell
		cout<<"Endpoint is in current cell"<<endl;
		Line goal_line(goal,goal);
		return NavLine(goal_line);
	}
	else{
		//int goal_cell_id=-1;
		//MeshCell* goal_cell=_meshdata->FindCell(goal,goal_cell_id);

		Point apex=goal;
		int act_cell_id=-1;
		//int loc_ind=-1; // local index of first node to be found in startphase
		unsigned int path_ind=0;
		Point point_left,point_right; // Nodes creatin the wedge
		MeshCell* act_cell=_meshdata->FindCell(start,act_cell_id);

		Point p_act_ref=act_cell->GetMidpoint();
		point_left=edge_path.at(0)->GetLeft(p_act_ref);
		point_right=edge_path.at(0)->GetRight(p_act_ref);

		//Test
		//print_path(edge_path);
		//cout<<"left: "<<point_left.toString()<<" right: "<<point_right.toString()<<endl;

		Point point_run_left=point_left;
		Point point_run_right=point_right;

		bool apex_found=false;
		// lengthen the funnel at side
		bool run_left=true,run_right=true;

		while(!apex_found){
			if(path_ind!=edge_path.size()){
				if(edge_path.at(path_ind)->GetCell1()==act_cell_id)
					act_cell_id=edge_path.at(path_ind)->GetCell2();
				else if (edge_path.at(path_ind)->GetCell2()==act_cell_id)
					act_cell_id=edge_path.at(path_ind)->GetCell1();
				else{
					Log->Write("ERROR:\t inconsistence between act_cell and edgepath");
					cout<<"act cell id="<<act_cell_id;
					cout<<"Cells 1 and 2 are"<<edge_path.at(path_ind)->GetCell1()<<"and "<<edge_path.at(path_ind)->GetCell2()<<endl;
					exit(EXIT_FAILURE);
				}
				act_cell=_meshdata->GetCellAtPos(act_cell_id);
				p_act_ref=act_cell->GetMidpoint();
				//find next points for wedge
				if(path_ind+1<edge_path.size()){ // Last Cell not yet reached =>Continue or node on edge
					if(run_left){
						point_run_left=edge_path.at(path_ind+1)->GetLeft(p_act_ref);
					}
					if(run_right){
						point_run_right=edge_path.at(path_ind+1)->GetRight(p_act_ref);
					}
				}else{//goal in actual cell => apex= goal or node on edge
					//cout<<"In else case!"<<endl;
					point_run_left=goal;
					point_run_right=goal;
				}
				// Test for new Points to be in the wedge of start
				int test_l=TestinFunnel(start,point_left,point_right,point_run_left);
				int test_r=TestinFunnel(start,point_left,point_right,point_run_right);

				if(test_l==0 && test_r==0){ //Narrow wedge on both sides
					//cout<<"narrow wedge on both sides"<<endl;
					point_left=point_run_left;
					point_right=point_run_right;

				}
				else if(test_l==1 && test_r==0){// narrow right side
					//cout<<"narrow right side"<<endl;
					point_right=point_run_right;

				}
				else if(test_l==0 && test_r==3){// narrow left side
					//cout<<"narrow left side"<<endl;
					point_left=point_run_left;

				}
				else if(test_l==1 && test_r==1){// apex=left
					//cout<<"apex=left"<<endl;
					apex=point_left;
					//return NavLine(Line(edge_path.at(path_ind)->GetPoint1(),edge_path.at(path_ind)->GetPoint2()));

					apex_found=true;
				}
				else if(test_l==3 && test_r==3){//apex=right
					//cout<<"apex=right"<<endl;
					apex=point_right;
					//return Nav	Line(Line(edge_path.at(path_ind)->GetPoint1(),edge_path.at(path_ind)->GetPoint2()));

					apex_found=true;
				}
				else if(test_l==1 && test_r==3){ //  Widen wedge
					//cout<<"widen wedge"<<endl;

				}
				else{// Corrupted data
					Log->Write("ERROR:\tFunnel reaches undefined state");
					exit(EXIT_FAILURE);
				}
				path_ind++;
			}else{//After some Funnel iterations the cell containing the goal is reached;
				//apex=goal; // Initialisation!
				//cout<<"Funnel progressed to goal and stopped"<<endl;
				apex_found=true;
				apex=edge_path.back()->GetPoint1();//Test
				//return NavLine(Line(edge_path.at(path_ind-1)->GetPoint1(),edge_path.at(path_ind-1)->GetPoint2()));
			}
		}//END WHILE
		//cout<<"Funnel from"<<start.toString()<<" results in "<<apex.toString()<<endl;

		// Some kind of workaround
		// First Edge which contains the found apex
		bool edgefound=false;
		path_ind=0;
		Point p1=apex,p2=apex;
		while(!edgefound){
			if(edge_path.at(path_ind)->GetPoint1()==apex){
				p1=apex;
				p2=edge_path.at(path_ind)->GetPoint2();
				edgefound=true;
			}
			else if(edge_path.at(path_ind)->GetPoint2()==apex){
				p1=apex;
				p2=edge_path.at(path_ind)->GetPoint1();
				edgefound=true;
			}
			path_ind++;
		}

		//NavLine exitline(Line(p1,p2));
		Point p1_new=(p1-p2)*0.9+p2;
		Point p2_new=(p2-p1)*0.9+p1;
		NavLine exitline(Line(p1_new,p2_new));
		//cout<<"Funnel: exitline: "<<exitline.toString()<<endl;
		return exitline;


	}//END IF
}

MeshEdge* MeshRouter::Visibility(Point& start,Point& goal,vector<MeshEdge*> edge_path)const{

	//return *(edge_path.begin());
	if(edge_path.empty()){
		exit(EXIT_FAILURE);
	}else{
		//cout<<start.toString()<<endl;

		int act_cell_id=-1;
		Point point_left,point_right; // Nodes creatin the wedge
		MeshCell* act_cell=_meshdata->FindCell(start,act_cell_id);

		Point p_act_ref=act_cell->GetMidpoint();
		point_left=edge_path.at(0)->GetLeft(p_act_ref);
		point_right=edge_path.at(0)->GetRight(p_act_ref);

		Point point_run_left=point_left;
		Point point_run_right=point_right;

		bool mesh_edge_found=false;

		MeshEdge* act_edge=edge_path.at(0);
		unsigned int mesh_pos=1;
		while(!mesh_edge_found && mesh_pos<edge_path.size()){
			//cout<<mesh_pos<<endl;

			point_run_left=edge_path.at(mesh_pos)->GetLeft(p_act_ref);
			point_run_right=edge_path.at(mesh_pos)->GetRight(p_act_ref);

			int test_l=TestinFunnel(start,point_left,point_right,point_run_left);
			int test_r=TestinFunnel(start,point_left,point_right,point_run_right);

			if(point_left==point_run_left)
				test_l=0;
			else if (point_right==point_run_right)
				test_r=0;

			if(test_l==0 && test_r==0){ //Narrow wedge on both sides
				//cout<<"narrow wedge on both sides"<<endl;
				point_left=point_run_left;
				point_right=point_run_right;
			}else{
				act_edge=edge_path.at(mesh_pos-1);
				mesh_edge_found=true;
			}
			mesh_pos++;
		}
		//cout<<"The next edge is: "<<act_edge->toString()<<endl;
		return act_edge;
		return edge_path.at(0);
	}
}

vector<MeshEdge*> MeshRouter::AStar(Pedestrian* p,int& status)const{

	// Path from start to goal through this edges
	vector<MeshEdge*> pathedge;

	int c_start_id;
	//Point testp_start(0,0);
	Point  p_start=p->GetPos();
	//cout<<testp_start.toString()<<endl;;
	MeshCell* start_cell=_meshdata->FindCell(p_start,c_start_id);
	if(start_cell!=NULL){
		//cout<<testp_start.toString()<<"Found in cell "<<c_start_id<<endl;
	}
	else{
		Log->Write("Startpoint not found");
		std::cout.precision(10);
		std::cout.setf( std::ios::fixed, std:: ios::floatfield );
		cout<<"startpoint: "<< p_start.GetX()<<" "<<p_start.GetY()<<"of pedestrian: "<<p->GetID()<<endl;
		exit(EXIT_FAILURE);
	}
	int c_goal_id;
	Point point_goal = _building->GetFinalGoal(p->GetFinalDestination())->GetCentroid();
	//cout<<"here"<<endl;
	MeshCell* goal_cell=_meshdata->FindCell(point_goal,c_goal_id);
	if(goal_cell!=NULL){
		//cout<<testp_goal.toString()<<"Found in cell: "<<c_goal_id<<endl;//
	}
	else{
		cout<<"Goal not found"<<endl;
	}


	//Initialisation
	unsigned int c_totalcount=_meshdata->GetCellCount();
	//cout<<"Total Number of Cells: "<<c_totalcount<<endl;
	bool* closedlist=new bool[c_totalcount];
	bool* inopenlist=new bool[c_totalcount];
	int* predlist=new int[c_totalcount]; // to gain the path from start to goal
	MeshEdge ** predEdgelist=new MeshEdge*[c_totalcount];
	double* costlist=new double[c_totalcount];
	for(unsigned int i=0;i<c_totalcount;i++){
		closedlist[i]=false;
		inopenlist[i]=false;
		predlist[i]=-1;
	}

	MeshCell* act_cell=start_cell;
	int act_id=c_start_id;
	double f= (act_cell->GetMidpoint()-point_goal).Norm();
	double act_cost=f;
	vector<pair< double , MeshCell*> > openlist;
	openlist.push_back(make_pair(f,start_cell));
	costlist[act_id]=f;
	inopenlist[c_start_id]=true;

	while(act_id!=c_goal_id){
		act_cost=costlist[act_id];
		//astar_print(closedlist,inopenlist,predlist,c_totalcount,act_id);
		if (act_cell==NULL)
			cout<<"act_cell=NULL !!"<<endl;

		for(unsigned int i=0;i<act_cell->GetEdges().size();i++){
			int act_edge_id=act_cell->GetEdges().at(i);
			MeshEdge* act_edge=_meshdata->GetEdges().at(act_edge_id);
			int nb_id=-1;
			// Find neighbouring cell

			if(act_edge->GetCell1()==act_id){
				nb_id=act_edge->GetCell2();
			}
			else if(act_edge->GetCell2()==act_id){
				nb_id=act_edge->GetCell1();
			}
			else{// Error: inconsistant
				Log->Write("Error:\tInconsistant Mesh-Data");
			}
			int n1_pos=act_edge->GetNode1();
			int n2_pos=act_edge->GetNode2();
			Point p1_p=*_meshdata->GetNodes().at(n1_pos);
			Point p2_p=*_meshdata->GetNodes().at(n2_pos);
			double length=(p1_p-p2_p).Norm();
			MeshCell* nb_cell=_meshdata->GetCellAtPos(nb_id);
			// Calculate
			if (nb_cell->GetEdges().size()==3){

			}
			if (!closedlist[nb_id] && length>0.2){// neighbour-cell not fully evaluated
				//MeshCell* nb_cell=_meshdata->GetCellAtPos(nb_id);
				double new_cost=act_cost+(act_cell->GetMidpoint()-nb_cell->GetMidpoint()).Norm();
				if(!inopenlist[nb_id]){// neighbour-cell not evaluated at all
					predlist[nb_id]=act_id;
					//predEdgelist[nb_id]=_meshdata->GetEdges().at(act_edge_id);
					predEdgelist[nb_id]=act_edge;
					costlist[nb_id]=new_cost;
					inopenlist[nb_id]=true;

					double f=new_cost+(nb_cell->GetMidpoint()-point_goal).Norm();
					openlist.push_back(make_pair(f,nb_cell));
				}
				else{
					if (new_cost<costlist[nb_id]){
						//cout<<"ERROR"<<endl;
						//found shorter path to nb_cell
						predlist[nb_id]=act_id;
						costlist[nb_id]=new_cost;
						// update nb in openlist
						for(unsigned int j=0;j<openlist.size();j++){
							if(openlist.at(i).second->GetID()==nb_id){
								MeshCell* nb_cell=openlist.at(i).second;
								double f=new_cost+(nb_cell->GetMidpoint()-point_goal).Norm();
								openlist.at(i)=make_pair(f,nb_cell);
								break;
							}
						}
					}
					else{
						// Do nothing: Path is worse
					}
				}
			}
		}

		vector<pair<double,MeshCell*> >::iterator it=openlist.begin();

		while(it->second->GetID()!=act_id){
			it++;
		}
		closedlist[act_id]=true;
		inopenlist[act_id]=false;
		openlist.erase(it);

		int next_cell_id=-1;
		MeshCell* next_cell=NULL;
		//astar_print(closedlist,inopenlist,predlist,c_totalcount,act_id,openlist); ///////////////
		if (openlist.size()>0){
			//Find cell with best f value
			double min_f=openlist.at(0).first;
			next_cell_id=openlist.at(0).second->GetID();
			//cout<<"next_cell_id: "<<next_cell_id<<endl;
			next_cell=openlist.at(0).second;
			for(unsigned int j=1;j<openlist.size();j++){
				if (openlist.at(j).first<min_f){
					min_f=openlist.at(j).first;
					next_cell=openlist.at(j).second;
					next_cell_id=openlist.at(j).second->GetID();
				}
			}
			act_id=next_cell_id;
			act_cell=next_cell;
		}
		else{
			Log->Write("Error:\tA* did not find a path");
		}
	}
	delete[] closedlist;
	delete[] inopenlist;
	delete[] costlist;
	//print_path(predlist,c_start_id,c_goal_id);/////////////////
	//astar_print(closedlist,inopenlist,predlist,c_totalcount,act_id,openlist);

	// In the case the agent is in the destination cell
	if(predlist[c_goal_id]==-1){
		status=-1;
		return pathedge;
	}

	// Building the path reversely from goal to start
	act_id=c_goal_id;
	while(predlist[act_id]!=c_start_id){
		pathedge.push_back(predEdgelist[act_id]);
		act_id=predlist[act_id];
	}

	if(predlist[act_id]==c_start_id)
		pathedge.push_back(predEdgelist[act_id]);

	delete[] predlist;
	delete[] predEdgelist;

	// Reverse the reversed path
	std::reverse(pathedge.begin(),pathedge.end());

	status=0;
	return pathedge;
}

int MeshRouter::FindExit(Pedestrian* p){
	//cout<<"calling the mesh router"<<endl;
	Point  point_start=p->GetPos();
	int c_start_id=-1;
	_meshdata->FindCell(point_start,c_start_id);
	MeshEdge* edge=NULL;
	NavLine* nextline=NULL;
	NavLine line;
	MeshEdge* meshline=NULL;


	if (false){// Compute the goal each update
	//if(p->GetCellPos()==c_start_id){
		nextline=p->GetExitLine();
	}else{
		int status=-1;
		//cout<<"before A*"<<endl;
		vector<MeshEdge*> edgepath=AStar(p,status);
		//cout<<"after A*"<<endl;
		if (status==-1) return -1;
		if(edgepath.empty()){
			Log->Write("Path is empty but next edge is defined");
			exit(EXIT_FAILURE);
		}

		//TODO: save the point goal in the ped class
		Point point_goal = _building->GetFinalGoal(p->GetFinalDestination())->GetCentroid();
		//cout<<"Goal: "<<point_goal.toString()<<endl;
		//line=Funnel(point_start,point_goal,edgepath);
		bool funnel=false;
		if(funnel){
			line=Funnel(point_start,point_goal,edgepath);
			if(line.GetPoint1()==line.GetPoint2()){
				Log->Write("ERROR:\tNavLine is a point");
				//cout<<"This point is: "<<line.GetPoint1().toString()<<endl;
				exit(EXIT_FAILURE);
			}else{
				//cout<<"The line is: "<<line.toString()<<endl;
			}
			nextline=&line;
		}else{
			meshline=Visibility(point_start,point_goal,edgepath);
			nextline=dynamic_cast<NavLine*>(meshline);
			Point p1=nextline->GetPoint1();
			Point p2=nextline->GetPoint2();
			//Point p1_new=(p1-p2)*0.9+p2;
			//Point p2_new=(p2-p1)*0.9+p1;
			nextline->SetPoint1((p1-p2)*0.9+p2);
			nextline->SetPoint2((p2-p1)*0.9+p1);

			//NavLine exitline(Line(p1_new,p2_new));
			/*
			if(p->GetID()==22){
				cout<<meshline->GetID()<<endl;
				print_path(edgepath);
			}*/
		}
		//cout<<"Goal"<<point_goal.toString()<<endl;
		//nextline=&line;

		//edge=*(edgepath.begin());
		//nextline=dynamic_cast<NavLine*>(edge);

		//Debug

		if(nextline==NULL){
			Log->Write("Edge is corrupt");
			exit(EXIT_FAILURE);
		}
		//cout<<"here"<<endl;

		//cout<<"nextline: "<<nextline->toString()<<endl;
	}// END ELSE

	p->SetExitLine(nextline);
	//p->SetCellPos(c_start_id);
	return 0;
}

void MeshRouter::FixMeshEdges(){
	for(unsigned int i=0;i<_meshdata->GetEdges().size();i++){

		MeshEdge* edge=_meshdata->GetEdges().at(i);
		for (map<int, Crossing*>::const_iterator itr = _building->GetAllCrossings().begin();
				itr != _building->GetAllCrossings().end(); ++itr) {

			//int door=itr->first;
			//int door = itr->second->GetUniqueID();
			Crossing* cross = itr->second;
			if(edge->operator ==(*cross)){
				edge->SetRoom1(cross->GetRoom1());
				edge->SetSubRoom1(cross->GetSubRoom1());
				edge->SetSubRoom2(cross->GetSubRoom2());

			}
		}
		for (map<int, Transition*>::const_iterator itr = _building->GetAllTransitions().begin();
				itr != _building->GetAllTransitions().end(); ++itr) {

			//int door=itr->first;
			//int door = itr->second->GetUniqueID();
			Transition* cross = itr->second;
			//const Point& centre = cross->GetCentre();
			//double center[2] = { centre.GetX(), centre.GetY() };
			if(edge->operator ==(*cross)){
				edge->SetRoom1(cross->GetRoom1());
				//edge->SetRoom2(cross->GetRoom2());
				edge->SetSubRoom1(cross->GetSubRoom1());
				edge->SetSubRoom2(cross->GetSubRoom2());
			}
		}
	}

	//	int  size=_meshdata->Get_outEdges().size();
	//	for(int i=0;i<size;i++){
	//		MeshEdge* edge=_meshdata->Get_outEdges().at(i);
	//		for (map<int, Crossing*>::const_iterator itr = _building->GetAllCrossings().begin();
	//				itr != _building->GetAllCrossings().end(); ++itr) {
	//
	//			//int door=itr->first;
	//			int door = itr->second->GetUniqueID();
	//			Crossing* cross = itr->second;
	//			if(edge->operator ==(*cross)){
	//				edge->SetRoom1(cross->GetRoom1());
	//				edge->SetSubRoom1(cross->GetSubRoom1());
	//				edge->SetSubRoom2(cross->GetSubRoom2());
	//
	//			}
	//		}
	//		for (map<int, Transition*>::const_iterator itr = _building->GetAllTransitions().begin();
	//				itr != _building->GetAllTransitions().end(); ++itr) {
	//
	////			int door=itr->first;
	//			int door = itr->second->GetUniqueID();
	//			Transition* cross = itr->second;
	//			const Point& centre = cross->GetCentre();
	//			double center[2] = { centre.GetX(), centre.GetY() };
	//			if(edge->operator ==(*cross)){
	//				edge->SetRoom1(cross->GetRoom1());
	//				edge->SetSubRoom1(cross->GetSubRoom1());
	//				edge->SetSubRoom2(cross->GetSubRoom2());
	//			}
	//		}
	//	}

	for(unsigned int i=0;i<_meshdata->GetEdges().size();i++){
		MeshEdge* edge=_meshdata->GetEdges().at(i);
		if(edge->GetRoom1()==NULL){

			for (int i = 0; i < _building->GetNumberOfRooms(); i++) {
				Room* room = _building->GetRoom(i);
				for (int j = 0; j < room->GetNumberOfSubRooms(); j++) {
					SubRoom* sub = room->GetSubRoom(j);
					if(sub->IsInSubRoom( edge->GetCentre())){
						edge->SetSubRoom1(sub);
						edge->SetSubRoom2(sub);
						edge->SetRoom1(room);
					}
				}
			}
		}
	}
	for(unsigned int i=0;i<_meshdata->GetEdges().size();i++){
		MeshEdge* edge=_meshdata->GetEdges().at(i);
		if(edge->GetRoom1()==NULL){
			exit(EXIT_FAILURE);
		}
	}
}

void MeshRouter::Init(Building* b) {
	_building=b;
	//Log->Write("WARNING: \tdo not use this  <<Mesh>>  router !!");

	string meshfileName=GetMeshFileName();
	ifstream meshfiled;
	meshfiled.open(meshfileName.c_str(), ios::in);
	if(!meshfiled.is_open()){
		Log->Write("ERROR: \tcould not open meshfile <%s>",meshfileName.c_str());
		exit(EXIT_FAILURE);
	}
	stringstream meshfile;
	meshfile<<meshfiled.rdbuf();
	meshfiled.close();

	vector<Point*> nodes; //nodes.clear();
	vector<MeshEdge*> edges;
	vector<MeshEdge*> outedges;
	vector<MeshCellGroup*> mCellGroups;

	unsigned int countNodes=0;
	meshfile>>countNodes;
	for(unsigned int i=0;i<countNodes;i++){
		double temp1,temp2;
		meshfile>>temp1>>temp2;
		nodes.push_back(new Point(temp1,temp2));
	}
	cout<<setw(2)<<"Read "<<nodes.size()<<" Nodes from file"<<endl;

	unsigned int countEdges=0;
	meshfile>>countEdges;
	for(unsigned int i=0;i<countEdges;i++){
		int t1,t2,t3,t4;
		meshfile>>t1>>t2>>t3>>t4;
		edges.push_back(new MeshEdge(t1,t2,t3,t4,*(nodes.at(t1)),*(nodes.at(t2))));
	}
	cout<<"Read "<<edges.size()<<" inner Edges from file"<<endl;

	unsigned int countOutEdges=0;
	meshfile>>countOutEdges;
	for(unsigned int i=0;i<countOutEdges;i++){
		int t1,t2,t3,t4;
		meshfile>>t1>>t2>>t3>>t4;
		outedges.push_back(new MeshEdge(t1,t2,t3,t4,*(nodes.at(t1)),*(nodes.at(t2))));
	}
	cout<<"Read "<<outedges.size()<<" outer Edges from file"<<endl;

	int tc_id=0;
	//int while_counter=0;//
	while(!meshfile.eof()){
		//cout<<"in while(!meshfile.eof()): "<<while_counter<<endl;
		string groupname;
		bool  namefound=false;
		//TODO better rouine for skipping empty lines
		while(!namefound && getline(meshfile,groupname)){
			if (groupname.size()>1){
				namefound=true;
				//cout<<"groupname: "<<groupname<<endl;
			}
		}
		if (!meshfile.eof()){

			unsigned int countCells=0;
			meshfile>>countCells;

			vector<MeshCell*> mCells; mCells.clear();
			for(unsigned int i=0;i<countCells;i++){
				double midx,midy;
				meshfile>>midx>>midy;
				unsigned int countNodes=0;
				meshfile>>countNodes;
				vector<int> node_id;
				for(unsigned int j=0;j<countNodes;j++){
					int tmp;
					meshfile>>tmp;
					node_id.push_back(tmp);
				}
				//double* normvec=new double[3];
				double normvec[3];
				for (unsigned int j=0;j<3;j++){
					double tmp=0.0;
					meshfile>>tmp;
					normvec[j]=tmp;
				}
				unsigned int countEdges=0;
				meshfile>>countEdges;
				vector<int> edge_id;
				for(unsigned int j=0;j<countEdges;j++){
					int tmp;
					meshfile>>tmp;
					edge_id.push_back(tmp);
				}
				unsigned int countWalls=0;
				meshfile>>countWalls;
				vector<int> wall_id;
				for(unsigned int j=0;j<countWalls;j++){
					int tmp;
					meshfile>>tmp;
					wall_id.push_back(tmp);
				}
				mCells.push_back(new MeshCell(midx,midy,node_id,normvec,edge_id,wall_id,tc_id));
				tc_id++;
			}
			mCellGroups.push_back(new MeshCellGroup(groupname,mCells));
		}
		//while_counter++;//
		//if(while_counter>50)//
		//	break;//
	}
	_meshdata=new MeshData(nodes,edges,outedges,mCellGroups);
	FixMeshEdges();
/*
	int found_cell;
	//_meshdata->FindCell(Point(14.5386998558,7.9135540711),found_cell);
	_meshdata->FindCell(Point(14,7.91),found_cell);
	double xl=14.0,xu=17.0,yl=7.0,yu=10.0;
	int n=30,m=30;
	int** field=new int*[n];
	for(int i=0;i<n;i++)
		field[i]=new int[m];

	for(int i=0;i<n;i++){
		for(int j=0;j<m;j++){
			int cell_id=-2;
			_meshdata->FindCell(Point(xl+(xu-xl)/(n-1)*i,yl+(yu-yl)/(m-1)*j),cell_id);
			cout<<cell_id<<" ";
		}
		cout<<endl;
	}


	cout<<found_cell<<endl;
	exit(EXIT_SUCCESS);*/
}


string MeshRouter::GetMeshFileName() const {

	TiXmlDocument doc(_building->GetPojectFilename());
	if (!doc.LoadFile()){
		Log->Write("ERROR: \t%s", doc.ErrorDesc());
		Log->Write("ERROR: \t could not parse the project file");
		exit(EXIT_FAILURE);
	}

	// everything is fine. proceed with parsing
	TiXmlElement* xMainNode = doc.RootElement();
	TiXmlNode* xRouters=xMainNode->FirstChild("route_choice_models");

	string mesh_file="";

	for(TiXmlElement* e = xRouters->FirstChildElement("router"); e;
			e = e->NextSiblingElement("router")) {

		string strategy=e->Attribute("description");

		if(strategy=="nav_mesh"){
			if (e->FirstChild("parameters")->FirstChildElement("mesh_file"))
				mesh_file=e->FirstChild("parameters")->FirstChildElement("mesh_file")->Attribute("file");
		}

	}
	return mesh_file;
}
