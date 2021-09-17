#include <iostream>
#include <vector>
#include <queue>
#include <list>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <utility>
#include <map>
#include <fstream>
#include <iomanip>

using namespace std;

double next_exp(double lambda, double max){
	double r,x;
	do{
		r = drand48();
		x = -log(r)/lambda;
	}while(x > max);
	return x;
}

void initialize(vector< vector<int> >& processes, vector< list<int> >& cpu_bursts, vector< list<int> >& IO_bursts, 
	int pid_num, int seed, double lambda, int up_bound, double& total_cpu_bursts, int& cpu_bursts_count){
	srand48(seed);
	processes.clear();
	cpu_bursts.clear();
	IO_bursts.clear();
	total_cpu_bursts = 0;
	cpu_bursts_count = 0;
	for (int i = 0; i < pid_num; ++i)
	{
		vector<int> temp{i,0,0,0};
		list<int> a,b;
		cpu_bursts.push_back(a);
		IO_bursts.push_back(b);
		temp[1] = (int)floor(next_exp(lambda, up_bound));
		temp[2] = (int)ceil(drand48()*100);
		cpu_bursts_count += temp[2];
		temp[3] = (int)ceil(1/lambda);
		processes.push_back(temp);
		for (int j = 0; j < temp[2] - 1; ++j)
		{
			double next_random_num1 = ceil(next_exp(lambda, up_bound));
			cpu_bursts[i].push_back((int)next_random_num1);
			total_cpu_bursts += next_random_num1;
			double next_random_num2 = ceil(next_exp(lambda, up_bound));
			IO_bursts[i].push_back(10*(int)next_random_num2);
		}
		double next_random_num3 = ceil(next_exp(lambda, up_bound));
		cpu_bursts[i].push_back((int)next_random_num3); // No IO burst for last cpu burst
		total_cpu_bursts += next_random_num3;
	}
}

void print_readyq(list<vector<int> >& ready_q, int skip){
	cout << "[Q ";
	list<vector<int> >::iterator itr1 = ready_q.begin();
	if ((int)ready_q.size() == skip)
	{
		cout << "empty";
		cout << "]" << endl;
		return;
	}
	for (int i = 0; i < skip; ++i)	itr1++;
	for (; itr1 != ready_q.end(); ++itr1)
	{
		cout << (char)((*itr1)[0]+65);
	}
	if (ready_q.size() == 0)
	{
		cout << "empty";
	}
	cout << "]" << endl;
}

bool compare_sjf(const vector<int>& v1, const vector<int>& v2){
	if ( v1[2] == v2[2] )
	{
		if (v1[0] == v2[0])
		{
			return v1[1] <= v2[1];
		}
		else return v1[0] < v2[0];
	}
	else{
		//cerr << "v1[2]: " << v1[2] << " < v2[2]: " << v2[2] << endl;
		return (v1[2] < v2[2]);
	}
}

void update_wait_time(map<int, int>& wait_time, int which){
	if (which != -1)
	{
		if (wait_time.find(which) == wait_time.end())
		{
			cerr << "update wait_time failed\n";
		}
		wait_time[which]++;
	}
	else{
		for (map<int, int>::iterator itr = wait_time.begin(); itr != wait_time.end(); itr++)
		{
			itr->second++;
		}		
	}
}

int main(int argc, char const *argv[])
{
	int time_limit = 1000;
	if (argc != 8)
	{
		cerr << "Wrong nunmber of arguments" << endl;
		//fprintf(stderr, "Wrong nunmber of arguments\n");
		return EXIT_FAILURE;
	}
	int pid_num = atoi(argv[1]);
	int seed = atoi(argv[2]);
	double lambda = atof(argv[3]);
	int up_bound = atoi(argv[4]);
	int t_cs = atoi(argv[5]);
	double alpha = atof(argv[6]);
	int t_slice = atoi(argv[7]);
	if (pid_num < 1 || pid_num > 26)
	{
		cerr << "Invalid number of processes" << endl;
		return EXIT_FAILURE;
	}
	if (lambda < 0)
	{
		cerr << "Invalid lambda" << endl;
		return EXIT_FAILURE;
	} // lin lin hao ke ai
	if (up_bound < 0)
	{
		cerr << "Invalid upper bound" << endl;
		return EXIT_FAILURE;
	}
	if (t_cs < 0 || t_cs%2 != 0)
	{
		cerr << "Invalid time for context switch" << endl;
		return EXIT_FAILURE;
	}
	if (alpha < 0 || alpha > 1)
	{
		cerr << "Invalid alpha" << endl;
		return EXIT_FAILURE;
	}
	if (t_slice < 0)
	{
		cerr << "Invalid time slice" << endl;
		return EXIT_FAILURE;
	}

	ofstream outfile("simout.txt");
	//initialize
	vector< vector<int> > processes; //(pid, arrival time, num of cpu bursts, tau)
	vector< list<int> > cpu_bursts;
	vector< list<int> > IO_bursts;
	double avg_cpu_bursts;
	int cpu_bursts_count;
	double total_cpu_bursts;
	initialize(processes, cpu_bursts, IO_bursts, pid_num, seed, lambda, up_bound, total_cpu_bursts, cpu_bursts_count);
	avg_cpu_bursts = total_cpu_bursts / cpu_bursts_count;
	//int preemptions_num = 0;
	int con_switch_num = 0;
	map<int, int> wait_time;
	vector< vector<int> > turnaround_time;
	
	//print
	for (unsigned int i = 0; i < processes.size(); ++i)
	{
		cout << "Process " << (char) (i+65) << " (arrival time " << processes[i][1] << " ms) ";
		if (processes[i][2] != 1)
		{
			cout << processes[i][2] << " CPU bursts (tau " << processes[i][3] << "ms)" <<endl;
		}
		else{
			cout << processes[i][2] << " CPU burst (tau " << processes[i][3] << "ms)" <<endl;
		}
	}
	cout << endl;



	//FCFS simulation
	//prepare
	map<int, int> temp_proc;// <pid, next event time >
	list<vector<int> > ready_q; // <pid, next event time >
	map<int, int> IO_q; // <pid, next event time >

	vector<int> current;
	vector<int> switching_outed;
	int switch_pause = 0;
	int t = 0;
	double avg_wait_time = 0;
	for ( int i = 0; i < (int)processes.size(); ++i)
	{
		temp_proc[i] = processes[i][1];
	}
	//start simulation 
	cout << "time " << t << "ms: Simulator started for FCFS [Q empty]" << endl;
	do{
		if (current.size() != 0 && current[1] == t)
		{
			if (IO_bursts[current[0]].size() == 0)
			{
				cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " terminated ";
				print_readyq(ready_q, 0);
				current.clear();
				switch_pause += t_cs;
			}
			else{
				if (cpu_bursts[current[0]].size() == 1)
				{
					if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " completed a CPU burst; 1 burst to go "; 
				}
				else {
					if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " completed a CPU burst; " << cpu_bursts[current[0]].size() << " bursts to go "; 
				}
				if (t<time_limit)	print_readyq(ready_q, 0);
				vector<int> temp(current);
				temp[1] = t + IO_bursts[temp[0]].front() + t_cs/2;
				IO_bursts[temp[0]].pop_front();
				IO_q[temp[0]] = temp[1];
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " switching out of CPU; will block on I/O until time " << temp[1] << "ms "; 
				switch_pause += t_cs;
				if (t<time_limit)	print_readyq(ready_q, 0);
				current.clear();
			}
		}
		//switch outed process entering cpu
		if(current.size() == 0 && switching_outed.size() > 0 &&t >= switching_outed[1] && switch_pause == 0){
			switching_outed[1] = t + cpu_bursts[switching_outed[0]].front();
			current = switching_outed;
			if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(switching_outed[0]+65) << " started using the CPU for " << cpu_bursts[switching_outed[0]].front() << "ms burst ";
			cpu_bursts[switching_outed[0]].pop_front();
			if (t<time_limit)	print_readyq(ready_q,0);
			switching_outed.clear();
			con_switch_num++;
		}

		//arriving process entering cpu
		int del = 0;
		for (list<vector<int> >::iterator itr = ready_q.begin(); itr != ready_q.end(); ++itr)
		{
			if(current.size() == 0 && t >= (*itr)[1] && switch_pause == 0){
				(*itr)[1] = t + cpu_bursts[(*itr)[0]].front();
				current = *itr;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)((*itr)[0]+65) << " started using the CPU for " << cpu_bursts[(*itr)[0]].front() << "ms burst ";
				cpu_bursts[(*itr)[0]].pop_front();
				del++;
				if (t<time_limit)	print_readyq(ready_q,del);
				con_switch_num++;
				break;
			}
		}
		for (int j = 0; j < del; ++j)
		{
			ready_q.pop_front();
		}
		//process completed I/O, added to ready_q
		vector<int> del_vec;
		for (map<int, int >::iterator itr = IO_q.begin(); itr != IO_q.end() ; ++itr)
		{
			if (itr->second == t)
			{
				vector<int> temp = {itr->first, itr->second + t_cs/2};
				ready_q.push_back(temp);
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(itr->first+65) << " completed I/O; added to ready queue ";
				if (t<time_limit)	print_readyq(ready_q,0);
				del_vec.push_back(itr->first);
				if ((int)current.size() == 0 && (int)ready_q.size() == 1 && switch_pause == 0)
				{
					switch_pause = t_cs/2;
				}//IO_q --> ready_q --> CPU
			}
		}
		for (int j = 0; j < (int)del_vec.size(); ++j)
		{
			IO_q.erase(del_vec[j]);
		}
		//process arrving to ready_q
		del_vec.clear();
		for (map<int, int>::iterator itr = temp_proc.begin(); itr != temp_proc.end(); ++itr)
		{
			if(itr->second == t) {
				if ((int)current.size() == 0 && (int)ready_q.size() == 0 && switch_pause == 0)
				{
					switch_pause += t_cs/2;
				}
				vector<int> temp = {itr->first, itr->second};
				ready_q.push_back(temp);
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(itr->first+65) << " arrived; added to ready queue ";
				del_vec.push_back(itr->first);
				if (t<time_limit)	print_readyq(ready_q,0);
				wait_time[itr->first] = 0;
			}
		}
		for (int j = 0; j < (int)del_vec.size(); ++j)
		{
			temp_proc.erase(del_vec[j]);
		}
		//context switch out finish
		if (switch_pause == t_cs/2)
		{
			if (ready_q.size()>0)
			{
				switching_outed = {ready_q.front()[0], ready_q.front()[1]};
				ready_q.pop_front();
			}
		}
		if (switch_pause>0) switch_pause--;
		t++;
		//update wait time
		for (list<vector<int> >::iterator itr = ready_q.begin(); itr != ready_q.end(); ++itr)
		{
			if (switch_pause > 0 && itr == ready_q.begin() && (int)current.size() != 0)
			{
				continue;
			}
			else	update_wait_time(wait_time, (*itr)[0]);
		}
	}while( ready_q.size() > 0 || IO_q.size() > 0 || current.size() > 0 || temp_proc.size() > 0 || switching_outed.size() > 0);
	cout << "time " << t+t_cs/2-1 << "ms: Simulator ended for FCFS [Q empty]" << endl;

	 
	for (map<int, int>::iterator itr = wait_time.begin(); itr != wait_time.end(); ++itr)
	{
		avg_wait_time += itr->second;
	}
	avg_wait_time = avg_wait_time / cpu_bursts_count;

	outfile << fixed << setprecision(3) << "Algorithm FCFS\n";
	outfile << fixed << setprecision(3) << "-- average CPU burst time: " << avg_cpu_bursts << " ms\n";
	outfile << fixed << setprecision(3) << "-- avg wait time is " << avg_wait_time << endl;
	outfile << fixed << setprecision(3) << "-- average turnaround time: " << avg_wait_time + t_cs + avg_cpu_bursts<< " ms\n";
	outfile << fixed << setprecision(3) << "-- total number of context switches: " << con_switch_num << endl;
	outfile << fixed << setprecision(3) << "-- total number of preemptions: " << 0 << endl;
	outfile << fixed << setprecision(3) << "CPU utilization: " << (total_cpu_bursts * 100) / t<< "%\n";

	cout << endl;
	//SJF simulation
	//prepare
	avg_cpu_bursts = 0;
	initialize(processes, cpu_bursts, IO_bursts, pid_num, seed, lambda, up_bound, total_cpu_bursts, cpu_bursts_count);
	avg_cpu_bursts = total_cpu_bursts / cpu_bursts_count;
	//preemptions_num = 0;
	con_switch_num = 0;
	wait_time.clear();
	turnaround_time.clear();
	current.clear();
	switching_outed.clear();
	switch_pause = 0;
	t = 0;
	int prev_cpu_burst = 0;
	map<int, vector<int> > temp_proc_short; // <pid, <next event time, tau> >
	map<int, vector<int> > IO_q_short;	// <pid, <next event time, tau> >
	list<vector<int> > ready_q_short;	// <pid, next event time, tau >

	for ( int i = 0; i < (int)processes.size(); ++i )
	{
		vector<int> temp = {processes[i][1],processes[i][3]};
		temp_proc_short[i] = temp;
	}
	//start simulation 
	cout << "time " << t << "ms: Simulator started for SJF [Q empty]" << endl;
	do{
		if (current.size() != 0 && current[1] == t)
		{
			if (IO_bursts[current[0]].size() == 0)
			{
				cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " terminated ";
				print_readyq(ready_q_short, 0);
				current.clear();
				switch_pause += t_cs;
				prev_cpu_burst = 0;
			}
			else{
				if (cpu_bursts[current[0]].size() == 1)
				{
					if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " (tau " << current[2] << "ms) completed a CPU burst; 1 burst to go "; 
				}
				else {
					if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " (tau " << current[2] << "ms) completed a CPU burst; " << cpu_bursts[current[0]].size() << " bursts to go "; 
				}
				if (t<time_limit) print_readyq(ready_q_short, 0);
				vector<int> temp(current);
				temp[1] = t + IO_bursts[temp[0]].front() + t_cs/2;	// add context switch in time
				int new_tau = ceil((double)(1 - alpha) * temp[2] + (double)alpha * prev_cpu_burst);
				if (t<time_limit)	cout << "time " << t << "ms: Recalculated tau from " << current[2] << "ms to " << new_tau << "ms for process " << (char)(current[0]+65) << " ";
				if (t<time_limit)	print_readyq(ready_q_short, 0);
				IO_bursts[temp[0]].pop_front();
				IO_q_short[temp[0]] = {temp[1], new_tau};
				prev_cpu_burst = 0;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) <<  " switching out of CPU; will block on I/O until time " << temp[1] << "ms "; 
				switch_pause += t_cs;
				if (t<time_limit)	print_readyq(ready_q_short, 0);
				current.clear();
			}
		}
		//switch outed process entering cpu
		if(current.size() == 0 && switching_outed.size() > 0 && t >= switching_outed[1] && switch_pause == 0){
			switching_outed[1] = t + cpu_bursts[switching_outed[0]].front();
			current = switching_outed;
			if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(switching_outed[0]+65) << " (tau " << current[2] << "ms) started using the CPU for " << cpu_bursts[switching_outed[0]].front() << "ms burst ";
			prev_cpu_burst = cpu_bursts[switching_outed[0]].front();
			cpu_bursts[switching_outed[0]].pop_front();
			if (t<time_limit)	print_readyq(ready_q_short,0);
			switching_outed.clear();
			con_switch_num++;
		}

		//arriving process entering cpu
		int del = 0;
		for (list<vector<int> >::iterator itr = ready_q_short.begin(); itr != ready_q_short.end(); ++itr)
		{
			if(current.size() == 0 && t >= (*itr)[1] && switch_pause == 0){
				(*itr)[1] = t + cpu_bursts[(*itr)[0]].front();
				current = *itr;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)((*itr)[0]+65) << " (tau " << current[2] << "ms) started using the CPU for " << cpu_bursts[(*itr)[0]].front() << "ms burst ";
				prev_cpu_burst = cpu_bursts[(*itr)[0]].front();
				cpu_bursts[(*itr)[0]].pop_front();
				del++;
				if (t<time_limit)	print_readyq(ready_q_short,del);
				con_switch_num++;
				break;
			}
		}
		for (int j = 0; j < del; ++j)
		{
			ready_q_short.pop_front();
		}
		//process completed I/O, added to ready_q
		vector<int> del_vec;
		for (map<int, vector<int> >::iterator itr = IO_q_short.begin(); itr != IO_q_short.end() ; ++itr)
		{
			if (itr->second[0] == t)
			{
				vector<int> temp = {itr->first, itr->second[0] + t_cs/2, itr->second[1]};
				ready_q_short.push_back(temp);
				ready_q_short.sort(compare_sjf);
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(itr->first+65) << " (tau " << itr->second[1] << "ms) completed I/O; added to ready queue ";
				if (t<time_limit)	print_readyq(ready_q_short,0);
				del_vec.push_back(itr->first);
				if ((int)current.size() == 0 && (int)ready_q_short.size() == 1 && switch_pause == 0)
				{
					switch_pause = t_cs/2;
				}//IO_q --> ready_q --> CPU
			}
		}
		for (int j = 0; j < (int)del_vec.size(); ++j)
		{
			IO_q_short.erase(del_vec[j]);
		}
		//process arrving to ready_q
		del_vec.clear();
		for (map<int, vector<int> >::iterator itr = temp_proc_short.begin(); itr != temp_proc_short.end(); ++itr)
		{
			if(itr->second[0] == t) {
				if ((int)current.size() == 0 && (int)ready_q_short.size() == 0 && switch_pause == 0)
				{
					switch_pause += t_cs/2;
				}
				vector<int> temp = {itr->first, itr->second[0], itr->second[1]};
				ready_q_short.push_back(temp);
				ready_q_short.sort(compare_sjf);
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(itr->first+65) << " (tau " << itr->second[1] << "ms) arrived; added to ready queue ";
				del_vec.push_back(itr->first);
				if (t<time_limit)	print_readyq(ready_q_short, 0);
				wait_time[itr->first] = 0;
			}
		}
		for (int j = 0; j < (int)del_vec.size(); ++j)
		{
			temp_proc_short.erase(del_vec[j]);
		}
		//context switch out finish
		if (switch_pause == t_cs/2)
		{
			if (ready_q_short.size()>0)
			{
				switching_outed = {ready_q_short.front()[0], ready_q_short.front()[1],ready_q_short.front()[2]};
				ready_q_short.pop_front();
			}
		}
		if (switch_pause > 0) switch_pause--;
		t++;
		//update wait time
		for (list<vector<int> >::iterator itr = ready_q_short.begin(); itr != ready_q_short.end(); ++itr)
		{
			if (switch_pause > 0 && itr == ready_q_short.begin() && (int)current.size() != 0)
			{
				continue;
			}
			else{
				update_wait_time(wait_time, (*itr)[0]);
			}	
		}
	}while( ready_q_short.size() > 0 || IO_q_short.size() > 0 || current.size() > 0 || temp_proc_short.size() > 0 || switching_outed.size() > 0);
	cout << "time " << t+t_cs/2-1 << "ms: Simulator ended for SJF [Q empty]" << endl << endl;

	avg_wait_time = 0;
	for (map<int, int>::iterator itr = wait_time.begin(); itr != wait_time.end(); ++itr)
	{
		avg_wait_time += itr->second;
	}
	avg_wait_time = avg_wait_time / cpu_bursts_count;

	outfile << fixed << setprecision(3) << "Algorithm SJF\n";
	outfile << fixed << setprecision(3) << "-- average CPU burst time: " << avg_cpu_bursts << " ms\n";
	outfile << fixed << setprecision(3) << "-- avg wait time is " << avg_wait_time << endl;
	outfile << fixed << setprecision(3) << "-- average turnaround time: " << avg_wait_time + t_cs + avg_cpu_bursts<< " ms\n";
	outfile << fixed << setprecision(3) << "-- total number of context switches: " << con_switch_num << endl;
	outfile << fixed << setprecision(3) << "-- total number of preemptions: " << 0 << endl;
	outfile << fixed << setprecision(3) << "CPU utilization: " << (total_cpu_bursts * 100) / t << "%\n";



	//SRT simulation
	//prepare
	avg_cpu_bursts = 0;
	initialize(processes, cpu_bursts, IO_bursts, pid_num, seed, lambda, up_bound, total_cpu_bursts, cpu_bursts_count);
	avg_cpu_bursts = total_cpu_bursts / cpu_bursts_count;
	//preemptions_num = 0;
	con_switch_num = 0;
	wait_time.clear();
	turnaround_time.clear();
	current.clear();
	switching_outed.clear();
	switch_pause = 0;
	t = 0;
	prev_cpu_burst = 0;
	temp_proc_short.clear(); // <pid, <next event time, tau, current burst time, remaining burst time> >
	IO_q_short.clear();	// <pid, <next event time, tau, current burst time, remaining burst time> >
	ready_q_short.clear();	// <pid, next event time, tau, current burst time, remaining burst time>
	for ( int i = 0; i < (int)processes.size(); ++i )
	{
		vector<int> temp = {processes[i][1],processes[i][3], -1, -1};
		temp_proc_short[i] = temp;
	}
	//start simulation 
	cout << "time " << t << "ms: Simulator started for SRT [Q empty]" << endl;
	do{
		// if (current.size()> 0 && t < 100)
		// {
		// 	cout <<(char)(current[0]+65) << " :  has event t: " << current[1] << endl;
		// }
		if (current.size() != 0 && current[1] == t)
		{
			if (IO_bursts[current[0]].size() == 0)
			{
				cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " terminated ";
				print_readyq(ready_q_short, 0);
				current.clear();
				switch_pause += t_cs;
				prev_cpu_burst = 0;
			}
			else{
				if (cpu_bursts[current[0]].size() == 1)
				{
					if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " (tau " << current[2] << "ms) completed a CPU burst; 1 burst to go "; 
				}
				else {
					if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) << " (tau " << current[2] << "ms) completed a CPU burst; " << cpu_bursts[current[0]].size() << " bursts to go "; 
				}
				if (t<time_limit) print_readyq(ready_q_short, 0);
				vector<int> temp(current);
				temp[1] = t + IO_bursts[temp[0]].front() + t_cs/2;	// add context switch in time
				int new_tau = ceil((double)(1 - alpha) * temp[2] + (double)alpha * prev_cpu_burst);
				if (t<time_limit)	cout << "time " << t << "ms: Recalculated tau from " << current[2] << "ms to " << new_tau << "ms for process " << (char)(current[0]+65) << " ";
				if (t<time_limit)	print_readyq(ready_q_short, 0);
				IO_bursts[temp[0]].pop_front();
				IO_q_short[temp[0]] = {temp[1], new_tau, temp[3], 0};
				prev_cpu_burst = 0;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(current[0]+65) <<  " switching out of CPU; will block on I/O until time " << temp[1] << "ms "; 
				switch_pause += t_cs;
				if (t<time_limit)	print_readyq(ready_q_short, 0);
				current.clear();
			}
		}
		//switch outed process entering cpu
		if(current.size() == 0 && switching_outed.size() > 0 && t >= switching_outed[1] && switch_pause == 0){
			if (switching_outed[3] == switching_outed[4] || switching_outed[4] == 0)
			{
				switching_outed[1] = t + cpu_bursts[switching_outed[0]].front();
				current = switching_outed;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(switching_outed[0]+65) << " (tau " << current[2] << "ms) started using the CPU for " << cpu_bursts[switching_outed[0]].front() << "ms burst ";
				prev_cpu_burst = cpu_bursts[switching_outed[0]].front();
				current[3] = prev_cpu_burst;
				current[4] = prev_cpu_burst;	
				cpu_bursts[switching_outed[0]].pop_front();
			}
			else{
				switching_outed[1] = t + switching_outed[4];
				current = switching_outed;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(switching_outed[0]+65) << " (tau " << current[2] << "ms) started using the CPU for remaining " << switching_outed[4] << "ms of " << switching_outed[3] << "ms burst ";
				prev_cpu_burst = switching_outed[3];
			}
			if (t<time_limit)	print_readyq(ready_q_short,0);
			switching_outed.clear();
			con_switch_num++;
		}
		//arriving process entering cpu
		int del = 0;
		for (list<vector<int> >::iterator itr = ready_q_short.begin(); itr != ready_q_short.end(); ++itr)
		{
			if(current.size() == 0 && t >= (*itr)[1] && switch_pause == 0){
				if ((*itr)[3] == (*itr)[4])
				{
					(*itr)[1] = t + cpu_bursts[(*itr)[0]].front();
					(*itr)[3] = cpu_bursts[(*itr)[0]].front();
					(*itr)[4] = cpu_bursts[(*itr)[0]].front();
				}
				current = *itr;
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)((*itr)[0]+65) << " (tau " << current[2] << "ms) started using the CPU for " << (*itr)[3] << "ms burst ";
				prev_cpu_burst = (*itr)[3];
				if ((*itr)[3] == (*itr)[4])
				{
					cpu_bursts[(*itr)[0]].pop_front();
					del++;
				}
				if (t<time_limit)	print_readyq(ready_q_short,del);
				con_switch_num++;
				break;
			}
		}
		for (int j = 0; j < del; ++j)
		{
			ready_q_short.pop_front();
		}
		//process completed I/O, added to ready_q
		vector<int> del_vec;
		for (map<int, vector<int> >::iterator itr = IO_q_short.begin(); itr != IO_q_short.end() ; ++itr)
		{
			if (itr->second[0] == t)
			{
				vector<int> temp = {itr->first, itr->second[0] + t_cs/2, itr->second[1], itr->second[2], itr->second[3]};
				ready_q_short.push_back(temp);
				ready_q_short.sort(compare_sjf);
				if (t<time_limit){
					cout << "time " << t << "ms: Process " << (char)(itr->first+65) << " (tau " << itr->second[1] << "ms) completed I/O; ";
					if ((int) current.size() > 0 && current[2] - ((t - (current[1] - current[3]) )) > itr->second[1])
					{
						cout << "preempting " << (char)(current[0]+65) << " ";
						print_readyq(ready_q_short,0);
						//preempting = true;
						current[4] -= (t - (current[1] - current[3]) );
						current[1] = t + t_cs/2;
						//vector<int> temp(current);
						ready_q_short.push_back(current);
						switch_pause = t_cs;
						current.clear();
						ready_q_short.sort(compare_sjf);
					}
					else{
					 	cout << "added to ready queue ";
						print_readyq(ready_q_short,0);	
					}
				}		
				del_vec.push_back(itr->first);
				if ((int)current.size() == 0 && (int)ready_q_short.size() == 1 && switch_pause == 0)
				{
					switch_pause = t_cs/2;
				}//IO_q --> ready_q --> CPU
			}
		}
		for (int j = 0; j < (int)del_vec.size(); ++j)
		{
			IO_q_short.erase(del_vec[j]);
		}
		//process arrving to ready_q
		del_vec.clear();
		for (map<int, vector<int> >::iterator itr = temp_proc_short.begin(); itr != temp_proc_short.end(); ++itr)
		{
			if(itr->second[0] == t) {
				if ((int)current.size() == 0 && (int)ready_q_short.size() == 0 && switch_pause == 0)
				{
					switch_pause += t_cs/2;
				}
				vector<int> temp = {itr->first, itr->second[0], itr->second[1], -1, -1};
				ready_q_short.push_back(temp);
				ready_q_short.sort(compare_sjf);
				if (t<time_limit)	cout << "time " << t << "ms: Process " << (char)(itr->first+65) << " (tau " << itr->second[1] << "ms) arrived; added to ready queue ";
				del_vec.push_back(itr->first);
				if (t<time_limit)	print_readyq(ready_q_short, 0);
				wait_time[itr->first] = 0;
			}
		}
		for (int j = 0; j < (int)del_vec.size(); ++j)
		{
			temp_proc_short.erase(del_vec[j]);
		}
		//context switch out finish
		if (switch_pause == t_cs/2)
		{
			if (ready_q_short.size()>0)
			{
				switching_outed = {ready_q_short.front()[0], ready_q_short.front()[1],ready_q_short.front()[2], ready_q_short.front()[3], ready_q_short.front()[4]};
				ready_q_short.pop_front();
			}
		}
		if (switch_pause > 0) switch_pause--;
		t++;
		//update wait time
		for (list<vector<int> >::iterator itr = ready_q_short.begin(); itr != ready_q_short.end(); ++itr)
		{
			if (switch_pause > 0 && itr == ready_q_short.begin() && (int)current.size() != 0)
			{
				continue;
			}
			else{
				update_wait_time(wait_time, (*itr)[0]);
			}	
		}
	}while( (ready_q_short.size() > 0 || IO_q_short.size() > 0 || current.size() > 0 || temp_proc_short.size() > 0 || switching_outed.size() > 0) && t < 39350);
	cout << "time " << t+t_cs/2-1 << "ms: Simulator ended for SRT [Q empty]" << endl;

	avg_wait_time = 0;
	for (map<int, int>::iterator itr = wait_time.begin(); itr != wait_time.end(); ++itr)
	{
		avg_wait_time += itr->second;
	}
	avg_wait_time = avg_wait_time / cpu_bursts_count;

	outfile << fixed << setprecision(3) << "Algorithm SRT\n";
	outfile << fixed << setprecision(3) << "-- average CPU burst time: " << avg_cpu_bursts << " ms\n";
	outfile << fixed << setprecision(3) << "-- avg wait time is " << avg_wait_time << endl;
	outfile << fixed << setprecision(3) << "-- average turnaround time: " << avg_wait_time + t_cs + avg_cpu_bursts<< " ms\n";
	outfile << fixed << setprecision(3) << "-- total number of context switches: " << con_switch_num << endl;
	outfile << fixed << setprecision(3) << "-- total number of preemptions: " << 0 << endl;
	outfile << fixed << setprecision(3) << "CPU utilization: " << (total_cpu_bursts * 100) / t<< "%\n";


	outfile.close();
	return 0;


	outfile.close();
	return 0;
}