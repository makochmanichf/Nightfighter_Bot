#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex>
#include <cmath>
#include <time.h>

using namespace std;

//constants
const long int maxncols=50,maxnrows=100,maxnbombers=5;	// maximum dimensions of map, maximum number of bombers
const long int maxnradars=3;				// maximum number of radars

//global variables
long int ncols,nrows;					// number of columns and rows in the map
long int start_hex;					// starting hex for NF, same as the Searchlight Beacon
long int nbombers;					// number of real bombers
long int nphantoms;					// number of phantom bombers
long int moon_phase;					// moon phase. 0 - no moon, 1 - half moon, 2 - full moon
long int visibility;					// visibility. 0 - poor, 1 - moderate, 2 - good
long int verbose;					// 0 - normal mode, 1 - no hidden information
long int global_earliest_entry,global_latest_entry;	// the earliest and latest possible turn of entry in the entire set of bombers
long int nturns;					// number of turns

// variables to do with the radar search and tallying
long int nradars;					// number of radar units dedicated to tracking the bombers
long int search_val[maxnradars];			// search values of the radars
long int ai_radar_range;				// this is 0, 1, or 2. 0 means no radar


long int max3(long int a, long int b, long int c)
{
	long int r;
	if ((a>=b) && (a>=c)) r=a;
	if ((b>=a) && (b>=c)) r=b;
	if ((c>=a) && (c>=b)) r=c;
	return r;
}

long int map_distance(long int h1,long int h2)	// this function returns the distance between hexes h1 and h2
						// the hex numbers must be entered without a leading zero
{
	long int a0,b0,a1,b1,x0,y0,x1,y1,dx,dy,dist;
	b0=int(floor(double(h1)/100.0))-1;
	a0=h1 % 100-1;
	b1=int(floor(double(h2)/100.0))-1;
	a1=h2 % 100-1;
	x0=a0-floor(double(b0)/2.0);
	y0=b0;
	x1=a1-floor(double(b1)/2.0);
	y1=b1;
	dx=x1-x0;
	dy=y1-y0;
	dist=max3(abs(dx),abs(dy),abs(dx+dy));
	return dist;
}

long int roll_d6(long int n)			// roll N D6's and return total
{
	long int i,result=0;
	for(i=0;i<=n-1;i++) result+=rand()%6+1;
	return result;
}

long int sixes(long int n)			// roll N D6's, then return the number of sixes rolled
{
	long int i,result=0;
	for(i=0;i<=n-1;i++) if ((rand()%6+1)==6) result+=1;
	return result;
}

class bomber
{
	public:
	long int position;			// current position of the bomber
	long int earliest_entry;		// earliest possible entry turn
	long int latest_entry;			// latest possible entry turn
	long int actual_entry;			// actual entry turn
	long int speed_odd;			// speed on odd-numbered turns, except on turn 1
	long int speed_first;			// speed on turn 1
	long int speed_even;			// speed on even-numbered turns
	bool present;				// is the bomber present on the map?
};

bomber real_bombers[maxnbombers];		// table with data on the real bombers
bomber phantom_bombers[maxnbombers];		// table with data on the real bombers

void read_input()
{
	cout<<"Game settings"<<endl;
	cout<<""<<endl;
	ifstream input;
	long int i;
	input.open("settings");
	input>>ncols>>nrows;
	//cout<<" ncols     = "<<ncols<<endl;
	//cout<<" nrows     = "<<nrows<<endl;
	cout<<" The hexes will be numbered from 101 to "<<ncols*100+nrows<<endl;
	cout<<" Please input the hex numbers without a leading zero, e.g. 103 not 0103"<<endl;
	input>>nturns;
	cout<<" number of turns in the game: "<<nturns<<endl;
	//cout<<" start_hex = "<<start_hex<<" (starting hex for the nightfighter)"<<endl;
	input>>nbombers;
	cout<<" number of bombers:           "<<nbombers<<endl;
	//read in the parameters of the bombers
	for(i=0;i<=nbombers-1;i++)
	{
		// bomber 1 earliest possible turn of entry	latest possible turn of entry		speed (odd-numbered turns)	speed (even-numbered turns)	DF	HP
		input>>real_bombers[i].earliest_entry;
		input>>real_bombers[i].latest_entry;
		input>>real_bombers[i].speed_odd;
		input>>real_bombers[i].speed_even;
		cout<<" Bomber "<<i<<":\n"<<"  earliest possible turn of entry: "<<real_bombers[i].earliest_entry<<"\n  latest possible turn of entry:   "<<real_bombers[i].latest_entry<<"\n  speed on odd-numbered turns:     "<<real_bombers[i].speed_odd<<"\n  speed on even-numbered turns:    "<<real_bombers[i].speed_even<<endl;
	}
	input>>verbose;
	input>>nradars;
	for(i=0;i<=nradars-1;i++) input>>search_val[i];
	input>>ai_radar_range;
	input.close();
	cout<<" nradars   = "<<nradars<<endl;
	cout<<" search values: ";
	for(i=0;i<=nradars-1;i++) cout<<search_val[i]<<" ";
	cout<<endl;
	if (ai_radar_range==0) cout<<" The fighter is not equipped with airborne interception (AI) radar."<<endl;
	else                   cout<<" Range of airborne interception (AI) radar: "<<ai_radar_range<<endl;
	
	cout<<endl;
}

void prepare_game()
{
	cout<<"Preparing the game"<<endl;
	long int i,j,turn_range;
	// initialize random seed:
	srand(time(NULL));
	// randomly determine visibility and moon phase
	i=roll_d6(2);
	//cout<<" Visibility roll (2D6) = "<<i<<endl;
	if (i>=10) visibility=2;
	else if (i>=5) visibility=1;
	else visibility=0;
	if (visibility==2) cout<<" Visibility is good 1/5"<<endl;
	if (visibility==1) cout<<" Visibility is moderate 0/3"<<endl;
	if (visibility==0) cout<<" Visibility is poor 0/1"<<endl;
	i=roll_d6(2)+2;
	//cout<<" Moon phase roll (2D6+2) = "<<i<<endl;
	if (i>=7) moon_phase=0;
	else if (i>=5) moon_phase=1;
	else moon_phase=2;
	if (moon_phase==2) cout<<" Moon phase: Full moon"<<endl;
	if (moon_phase==1) cout<<" Moon phase: Half moon"<<endl;
	if (moon_phase==0) cout<<" Moon phase: No moon"<<endl;

	// randomly determine turns of entry and entry hexes
	for(i=0;i<=nbombers-1;i++)
	{
		turn_range=real_bombers[i].latest_entry-real_bombers[i].earliest_entry;
		//cout<<"Bomber "<<i<<" : turn range is "<<turn_range<<endl;
		real_bombers[i].actual_entry=real_bombers[i].earliest_entry + (rand() % (turn_range+1));
		if (verbose==1) cout<<" Bomber "<<i<<" will enter on turn "<<real_bombers[i].actual_entry<<endl;
	}
	
	// on turn 1, a bomber may move fewer hexes than its normal speed
	for(i=0;i<=nbombers-1;i++)
	{
		real_bombers[i].speed_first=(rand() % real_bombers[i].speed_odd) + 1;
		if (verbose==1) cout<<" On turn 1, bomber "<<i<<" will move "<<real_bombers[i].speed_first<<" hexes."<<endl;
	}

	// generate phantom bombers
	nphantoms=nbombers+1;
	
	// the 0-th phantom bomber always enters on turn 1
	// in other respects, the 0-th phantom bomber is a copy of the 0-th real bomber
	phantom_bombers[0].actual_entry=1;
	phantom_bombers[0].speed_odd=real_bombers[0].speed_odd;
	phantom_bombers[0].speed_even=real_bombers[0].speed_even;
	// the other phantom bombers are also copies of real bombers
	for(i=1;i<=nphantoms-1;i++)
	{
		phantom_bombers[i].speed_odd= real_bombers[i-1].speed_odd;
		phantom_bombers[i].speed_even=real_bombers[i-1].speed_even;
	}
	
	// the other phantom bombers enter on random turns
	global_earliest_entry=100;
	global_latest_entry=0;
	for(i=0;i<=nbombers-1;i++)
	{
		if (global_earliest_entry>real_bombers[i].earliest_entry) global_earliest_entry=real_bombers[i].earliest_entry;
		if (global_latest_entry<  real_bombers[i].latest_entry)   global_latest_entry=  real_bombers[i].latest_entry;
	}
	//cout<<" global_earliest_entry = "<<global_earliest_entry<<endl;
	//cout<<" global_latest_entry   = "<<global_latest_entry<<endl;

	// randomly determine turns of entry and entry hexes for the phantom bombers
	if (verbose==1) cout<<" Phantom bomber "<<0<<" will enter on turn "<<phantom_bombers[0].actual_entry<<endl;
	for(i=1;i<=nphantoms-1;i++)
	{
		turn_range=global_latest_entry-global_earliest_entry;
		//cout<<"Bomber "<<i<<" : turn range is "<<turn_range<<endl;
		if (i<nphantoms-1)  phantom_bombers[i].actual_entry= 1 + (rand() % (turn_range+1));
		if (i==nphantoms-1) phantom_bombers[i].actual_entry= 1 + global_latest_entry;
		if (verbose==1) cout<<" Phantom bomber "<<i<<" will enter on turn "<<phantom_bombers[i].actual_entry<<endl;
	}
	
	// on turn 1, a phantom bomber may move fewer hexes than its normal speed
	for(i=0;i<=nphantoms-1;i++)
	{
		phantom_bombers[i].speed_first=(rand() % phantom_bombers[i].speed_odd) + 1;
		if (verbose==1) cout<<" On turn 1, phantom bomber "<<i<<" will move "<<phantom_bombers[i].speed_first<<" hexes."<<endl;
	}

	// generate the initial positions of all bombers
	// The bombers "spawn" in hexes from 200 to 100*(ncols-1)
	// The number of entry columns is ncols-2
	long int ientry,nentry=ncols-2;
	// real bombers
	for(i=0;i<=nbombers-1;i++)
	{
		ientry=(rand() % nentry)*100+200;
		if (verbose==1) cout<<" Bomber "<<i<<" will spawn in hex "<<ientry<<endl;
		real_bombers[i].position=ientry;
	}
	// phantom bombers
	for(i=0;i<=nphantoms-1;i++)
	{
		ientry=(rand() % nentry)*100+200;
		if (verbose==1) cout<<" Phantom bomber "<<i<<" will spawn in hex "<<ientry<<endl;
		phantom_bombers[i].position=ientry;
	}

	// at the start of the game, the bombers are not present on the map
	for(i=0;i<=nbombers-1;i++)  real_bombers[i].present=   false;
	for(i=0;i<=nphantoms-1;i++) phantom_bombers[i].present=false;
	
	cout<<endl;
}

void game_turn(long int turn)
{
	cout<<"Turn "<<turn<<endl;
	long int i,j,k,l;
	long int nf_position,nf_facing;
	long int search_hex[maxnradars];
	bool     contact[maxnradars];		//has the radar search made contact?
	long int sweep_hex[maxnradars];		//the hex where the sweep counter will be placed
	long int eligible[6];			//list of eligible hexes
	long int eligible_distance[6];		//shortest distance between a bomber and the given eligible hex
	long int nroll,roll[100],roll_result;	//array used to randomize the placement of the sweep counter
	long int min_d;				//shortest distance between a bomber and a sweep counter
	bool real_present=false,phantom_present=false;
	// Phase 1. Bombers move
	cout<<" Phase 1: Bombers move."<<endl;
	// Real bombers
	for(i=0;i<=nbombers-1;i++)
	if (real_bombers[i].actual_entry<=turn) 
	{
		if (real_bombers[i].actual_entry==turn) real_bombers[i].present=true;
		if ((turn % 2)==0)               real_bombers[i].position+=real_bombers[i].speed_even;
		if ((turn>1) && ((turn % 2)==1)) real_bombers[i].position+=real_bombers[i].speed_odd;
		if (turn==1)                     real_bombers[i].position+=real_bombers[i].speed_first;
		if (verbose==1) if (real_bombers[i].present) cout<<"  Bomber "<<i<<" position "<<real_bombers[i].position<<endl;
		// check if the bomber has left the map
		if ((real_bombers[i].position % 100 > nrows) && (real_bombers[i].present==true))
		{
			real_bombers[i].present=false;
			if (verbose==1) cout<<"Bomber "<<i<<" has left the map."<<endl;
		}

	}
	// Phantom bombers
	for(i=0;i<=nphantoms-1;i++)
	if (phantom_bombers[i].actual_entry<=turn) 
	{
		if (phantom_bombers[i].actual_entry==turn) phantom_bombers[i].present=true;
		if ((turn % 2)==0)               phantom_bombers[i].position+=phantom_bombers[i].speed_even;
		if ((turn>1) && ((turn % 2)==1)) phantom_bombers[i].position+=phantom_bombers[i].speed_odd;
		if (turn==1)                     phantom_bombers[i].position+=phantom_bombers[i].speed_first;
		if (verbose==1) if (phantom_bombers[i].present) cout<<"  Phantom bomber "<<i<<" position "<<phantom_bombers[i].position<<endl;
		// check if the bomber has left the map
		if ((phantom_bombers[i].position % 100 > nrows) && (phantom_bombers[i].present=true))
		{
			phantom_bombers[i].present=false;
			if (verbose==1) cout<<"Phantom bomber "<<i<<" has left the map."<<endl;
		}

	}
	
	// Phase 2. Fighters move
	cout<<" Phase 2: Fighter moves."<<endl;
	cout<<"  Enter the fighter's position and facing:"<<endl;
	//facing is 1 if the fighter is moving in the same direction as the incoming bombers. 0 otherwise
	cin>>nf_position>>nf_facing;
	cout<<"  nf_position = "<<nf_position<<endl;	
	cout<<"  nf_facing   = "<<nf_facing<<endl;

	// Phase 3. Radar search
	// The player directs the ongoing radar search by entering the search hexes
	cout<<" Phase 3: Ground radar search."<<endl;
	cout<<" The radar search values are as follows: ";
	if (nradars==1) cout<<search_val[0];
	if (nradars==2) cout<<search_val[0]<<", "<<search_val[1];
	if (nradars==3) cout<<search_val[0]<<", "<<search_val[1]<<", "<<search_val[2];
	cout<<endl;	
	cout<<"  Enter the radar search hexes:"<<endl;
	if (nradars==1) cin>>search_hex[0];
	if (nradars==2) cin>>search_hex[0]>>search_hex[1];
	if (nradars==3) cin>>search_hex[0]>>search_hex[1]>>search_hex[2];
	cout<<"  search hexes: ";
	for(i=0;i<=nradars-1;i++) cout<<search_hex[i]<<" ("<<search_val[i]<<") ";
	cout<<endl;
	
	//carry out the radar search
	for(i=0;i<=nradars-1;i++)
	{
		contact[i]=false;
		for(j=0;j<=nbombers-1;j++)	//loop over real bombers
		if (real_bombers[j].present==true)
		if (map_distance(real_bombers[j].position,search_hex[i])<=search_val[i]) contact[i]=true;
	}
	
	//For those radars that did not make contact, place sweep counters
	for(i=0;i<=nradars-1;i++)
	if (contact[i]==false)
	{
		//determine the six eligible hexes
		eligible[0]=search_hex[i]-1;
		eligible[1]=search_hex[i]+1;
		//IF WE ARE IN A ROW STARTING WITH AN ODD NUMBER (e.g. hex 1517)
		//THE LAST 4 NUMBERS MUST BE -101 +99 -100 +100
		if (int(floor(double(search_hex[i])*0.01)) % 2==1)
		{
			eligible[2]=search_hex[i]-101;
			eligible[3]=search_hex[i]+99;
			eligible[4]=search_hex[i]-100;
			eligible[5]=search_hex[i]+100;
		}
		//IF WE ARE IN A ROW STARTING WITH AN EVEN NUMBER (e.g. hex 1617)
		//THE LAST 4 NUMBERS MUST BE +101 -99 -100 +100
		if (int(floor(double(search_hex[i])*0.01)) % 2==0)
		{
			eligible[2]=search_hex[i]+101;
			eligible[3]=search_hex[i]-99;
			eligible[4]=search_hex[i]-100;
			eligible[5]=search_hex[i]+100;
		}
		if (verbose==1) cout<<"  Sweep for radar "<<i<<": the six eligible hexes are "<<eligible[0]<<" "<<eligible[1]<<" "<<eligible[2]<<" "<<eligible[3]<<" "<<eligible[4]<<" "<<eligible[5]<<" "<<endl;
		//determine the situation.
		for(j=0;j<=nbombers-1;j++)  if (real_bombers[j].present)    real_present=true;
		for(j=0;j<=nphantoms-1;j++) if (phantom_bombers[j].present) phantom_present=true;
		//Case 1. There are only phantom bombers on the map
		if ((real_present==false) && (phantom_present==true)) 
		{
			if (verbose==1) cout<<"   Case 1: Only phantom bombers are present."<<endl;
			//we place the sweep counter in one of the hexes that are the closest to a phantom bomber
			for(j=0;j<=5;j++) eligible_distance[j]=10000;
			for(j=0;j<=5;j++)		//j runs over hexes
			for(k=0;k<=nphantoms-1;k++)	//k runs over phantom bombers
			if (phantom_bombers[k].present==true)
			if (map_distance(eligible[j],phantom_bombers[k].position)<eligible_distance[j])
			eligible_distance[j]=map_distance(eligible[j],phantom_bombers[k].position);
			//print out the distances
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the distances to the closest phantom bomber are "<<eligible_distance[0]<<" "<<eligible_distance[1]<<" "<<eligible_distance[2]<<" "<<eligible_distance[3]<<" "<<eligible_distance[4]<<" "<<eligible_distance[5]<<" "<<endl;
			//determine the shortest distance ("min_d") between an eligible hex and a phantom bomber
			min_d=10000;
			for(j=0;j<=5;j++) if (min_d>eligible_distance[j]) min_d=eligible_distance[j];
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the shortest distance is "<<min_d<<endl;
			nroll=0;
			for(j=0;j<=5;j++) if (min_d==eligible_distance[j])
			{
				roll[nroll]=eligible[j];
				nroll++;
			}
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": nroll = "<<nroll<<" , roll : ";
			if (verbose==1) for(j=0;j<=nroll-1;j++) cout<<roll[j]<<" ";
			cout<<endl;
			roll_result=(rand() % nroll);
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": roll_result = "<<roll_result<<endl;
			sweep_hex[i]=roll[roll_result];
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the sweep counter will be placed in hex "<<sweep_hex[i]<<endl;			
			
		}
		//Case 2. There are only real bombers on the map - this is impossible, because we ensure that the last phantom bomber always enters after the last real bomber
		if ((real_present==true) && (phantom_present==false)) 
		{
			if (verbose==1) cout<<"   Case 2: Only real bombers are present."<<endl;
			if (verbose==1) cout<<"   THIS SITUATION SHOULD NEVER ARISE"<<endl;
		}
		//Case 3. There are both real bombers and phantom bombers
		if ((real_present==true) && (phantom_present==true)) 
		{
			if (verbose==1) cout<<"   Case 3: Both real and phantom bombers are present."<<endl;
			//we place the sweep counter in one of the hexes that are the closest to a real bomber
			for(j=0;j<=5;j++) eligible_distance[j]=10000;
			for(j=0;j<=5;j++)		//j runs over hexes
			for(k=0;k<=nbombers-1;k++)	//k runs over real bombers
			if ((real_bombers[k].present==true) && (map_distance(eligible[j],real_bombers[k].position)<eligible_distance[j]))
			eligible_distance[j]=map_distance(eligible[j],real_bombers[k].position);
			//print out the distances
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the distances to the closest real bomber are "<<eligible_distance[0]<<" "<<eligible_distance[1]<<" "<<eligible_distance[2]<<" "<<eligible_distance[3]<<" "<<eligible_distance[4]<<" "<<eligible_distance[5]<<" "<<endl;
			//determine the shortest distance ("min_d") between an eligible hex and a real bomber
			min_d=10000;
			for(j=0;j<=5;j++) if (min_d>eligible_distance[j]) min_d=eligible_distance[j];
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the shortest distance is "<<min_d<<endl;
			nroll=0;
			for(j=0;j<=5;j++) if (min_d==eligible_distance[j])
			{
				roll[nroll]=eligible[j];
				nroll++;
			}
			// if one one of the hexes listed in "roll" is located at a distance of exactly min_d to a phantom bomber, then the probability of that hex being selected increases.
			// This is because the umpire would normally try to throw off the nightfighter by presenting misleading information
			l=nroll;
			for(j=0;j<=l-1;j++)	//loop over the hexes listed in "roll"
			for(k=0;k<=nphantoms-1;k++)
			if ((phantom_bombers[k].present==true) && (map_distance(phantom_bombers[k].position,roll[j])==min_d)) //this is the condition for that to happen
			{
				roll[nroll]=roll[j];
				nroll++;
				if (verbose==1) cout<<"  Sweep for radar "<<i<<": it so happens that phantom bomber "<<k<<" in hex "<<phantom_bombers[k].position<<endl<<"  is located at the same distance to hex "<<roll[j]<<" as the closest real bomber."<<endl;
			}
			
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": nroll = "<<nroll<<" , roll : ";
			if (verbose==1) for(j=0;j<=nroll-1;j++) cout<<roll[j]<<" ";
			cout<<endl;
			roll_result=(rand() % nroll);
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": roll_result = "<<roll_result<<endl;
			sweep_hex[i]=roll[roll_result];
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the sweep counter will be placed in hex "<<sweep_hex[i]<<endl;
		}
		//Case 4. There are neither real nor phantom bombers on the map
		if ((real_present==false) && (phantom_present==false)) 
		{
			if (verbose==1) cout<<"   Case 4: Neither real nor phantom bombers are present."<<endl;
			if (verbose==1) cout<<"   THIS MEANS THE GAME IS EFFECTIVELY OVER"<<endl;
			//randomly select one of the adjacent hexes as the sweep hex
			eligible[0]=search_hex[i]-1;
			eligible[1]=search_hex[i]+1;
			//IF WE ARE IN A ROW STARTING WITH AN ODD NUMBER
			//THE LAST 4 NUMBERS MUST BE -101 +99 -100 +100
			if (int(floor(double(search_hex[i])*0.01)) % 2==1)
			{
				eligible[2]=search_hex[i]-101;
				eligible[3]=search_hex[i]+99;
				eligible[4]=search_hex[i]-100;
				eligible[5]=search_hex[i]+100;
			}
			//IF WE ARE IN A ROW STARTING WITH AN EVEN NUMBER
			//THE LAST 4 NUMBERS MUST BE +101 -99 -100 +100
			if (int(floor(double(search_hex[i])*0.01)) % 2==0)
			{
				eligible[2]=search_hex[i]+101;
				eligible[3]=search_hex[i]-99;
				eligible[4]=search_hex[i]-100;
				eligible[5]=search_hex[i]+100;
			}
			roll_result=(rand() % nroll);
			sweep_hex[i]=eligible[roll_result];
			if (verbose==1) cout<<"  Sweep for radar "<<i<<": the sweep counter will be placed in hex "<<sweep_hex[i]<<endl;
		}
	}
	
	//present results
	for(i=0;i<=nradars-1;i++)
	{
		cout<<"  Radar search in "<<search_hex[i]<<"("<<search_val[i]<<") : ";
		if (contact[i]==true) cout<<"CONTACT "; else cout<<"NO CONTACT, place a sweep counter at hex "<<sweep_hex[i];
		cout<<endl;
	}
	
	// Phase 4. Automatic AI radar search
	cout<<" Phase 4: AI radar search."<<endl;
	long int n_ai_hexes,ai_hexes[30]; //a list of hexes in which the nightfighter will be conducting the AI radar search
	//the radar search is only possible if the nightfighter is oriented in the same direction as the bombers
	//otherwise the speed mismatch is too high
	if ((ai_radar_range>0) && (nf_facing==1))
	{
		n_ai_hexes=ai_radar_range+1;
		ai_hexes[0]=nf_position;
		ai_hexes[1]=nf_position+1;
		if (ai_radar_range>=2) ai_hexes[2]=nf_position+2;
		if (ai_radar_range>=3) ai_hexes[3]=nf_position+3;

		for(i=0;i<=n_ai_hexes-1;i++)
		{
			for(j=0;j<=nbombers-1;j++) //loop over real bombers
			if (real_bombers[j].position==ai_hexes[i])
			if (real_bombers[j].present==true)
			cout<<"  AI radar search: CONTACT in hex "<<ai_hexes[i]<<endl;
		}
	}
	// Phase 5. Automatic tally phase
	cout<<" Phase 5: Tally phase."<<endl;
	long int ntally,tally_outcome;

	// The base number of tally dice depends on visibility.

	// if (visibility==2) cout<<" Visibility is good 1/5"<<endl;			=> 3 dice
	// if (visibility==1) cout<<" Visibility is moderate 0/3"<<endl;		=> 2 dice
	// if (visibility==0) cout<<" Visibility is poor 0/1"<<endl;			=> 1 die
	// plus, almost all German nightfighters had a good view from the cockpit	=> 1 extra die
	
	if (visibility==2)	ntally=4;
	else if (visibility==1)	ntally=3;
	else if (visibility==0)	ntally=2;

	// if (moon_phase==2) cout<<" Moon phase: Full moon"<<endl;			=> 1 extra die
	// if (moon_phase==1) cout<<" Moon phase: Half moon"<<endl;			=> 1 extra die
	// if (moon_phase==0) cout<<" Moon phase: No moon"<<endl;			=> no extra dice
	if ((moon_phase==2) || (moon_phase==1)) ntally+=1;

	// if the nightfighter is equipped with AI radar, it gets 2 bonus dice (in the original rules, that's 3 dice, but there are negative modifiers, so let's say 2 dice)
	if (ai_radar_range>0) ntally+=2;

	cout<<"  The nightfighter gets "<<ntally<<" tally dice."<<endl;
	
	tally_outcome=sixes(ntally);
	
	if ((tally_outcome>0) && (nf_facing==1))
	{
		// the nightfighter might potentially be able to spot a bomber
		// if visibility is good, the nightfighter can see a bomber in its own hex, and the hex immediately ahead
		// otherwise, only in its own hex
		if (visibility==2)
		{
			n_ai_hexes=2;
			ai_hexes[0]=nf_position;
			ai_hexes[1]=nf_position+1;
		}
		else
		{
			n_ai_hexes=1;
			ai_hexes[0]=nf_position;
		}

		for(i=0;i<=n_ai_hexes-1;i++)
		{
			for(j=0;j<=nbombers-1;j++)	//loop over real bombers
			if (real_bombers[j].position==ai_hexes[i]) 
			if (real_bombers[j].present==true)
			{
				cout<<"  TALLY on a bomber in hex "<<ai_hexes[i]<<endl;
				cout<<"  Mark the bomber on the map."<<endl;
			}
		}

	}

	cout<<endl;
}

void main_loop()
{
	cout<<"Starting the game..."<<endl;
	long int i,j,k;
	for(i=1;i<=nturns;i++) game_turn(i);	// note that the turns are numbered 1, 2, ..., nturns
}

int main(void)
{
        read_input();
        prepare_game();
        main_loop();
        return 0;
}
