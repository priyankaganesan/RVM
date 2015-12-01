#include "rvm.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <dirent.h>

using namespace std;

trans_t global_id;
map<trans_t, transaction_info> transactions_map;

void apply_log_for_segment(rvm_t rvm, string segname)
{
	string seg_file_path = rvm->file_path + "/" + segname;
	string log_file_path = seg_file_path + ".log";
	int seg_file, log_file;
	int offset, size;
	void *value;
	int size_of_int = sizeof(int);
	seg_file = open(seg_file_path.c_str(), O_RDWR | O_CREAT, 0755);
	if(seg_file == -1)
	{
		printf("Error opening segment file\n");
		return;
	}
	log_file = open(log_file_path.c_str(), O_RDWR | O_CREAT, 0755);
	if(log_file == -1)
	{
		close(seg_file);
		printf("Error opening log file\n");
		return;
	}
	int file_size = lseek(log_file, 0, SEEK_END);
	if(file_size <= 0)
		return;
	lseek(log_file, 0, 0);
	while(lseek(log_file, 0, SEEK_CUR) < file_size)
	{
		read(log_file, &offset, size_of_int);
		read(log_file, &size, size_of_int);
		value = operator new(size);
		read(log_file, value, size);
		lseek(seg_file, offset, 0);
		write(seg_file, value, size);
		operator delete(value);
	}
	close(seg_file);
	close(log_file);
	string cmd = "rm " + log_file_path;
	system(cmd.c_str());
	cmd = "touch " + log_file_path;
	system(cmd.c_str());
}

rvm_t rvm_init(const char *directory)
{
    struct stat sbuf;
    rvm_t rvm = new rvm_struct;
    rvm->file_path=string(directory);    
    if (stat(directory, &sbuf) == 0 && S_ISDIR(sbuf.st_mode))
    {
       cout<<"Directory already exists"<<endl;
       return rvm;   
    }
    char dir_str[100];
    strcpy(dir_str, "mkdir ");
    strcat(dir_str, directory);
    system(dir_str);
    return rvm;
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
	char seg_file[100];
	int seg_fd, seg_size;
	int extend_res;

	list<segment_t>::iterator seg_it;
	for(seg_it = rvm->seg_list.begin(); seg_it != rvm->seg_list.end(); seg_it++)
	{
		if(seg_it->segname == string(segname)){
			if(seg_it->is_valid == 1)
			{
				return NULL;
			}
			break;
		}
	}
	if(seg_it != rvm->seg_list.end())
	{
		return NULL;
	}

	apply_log_for_segment(rvm, string(segname));

	strcpy(seg_file, rvm->file_path.c_str());
	strcat(seg_file, "/");
	strcat(seg_file, segname);

	if (!(seg_fd = open(seg_file, O_RDWR | O_CREAT, 0755))) 
	{  
	    cout<<endl<<"Cannot open seg_file";
	    return NULL;
	}

	if ((seg_size = lseek(seg_fd, 0, SEEK_END)) < 0) 
	{
        close(seg_fd);
        cout<<endl<<"Cannot get seg_size";
        return NULL;
    }

    /*
     * Trying to extend segment
     */
    if (seg_size < size_to_create)
    {
        lseek(seg_fd, size_to_create - 1, SEEK_SET);
        extend_res = write(seg_fd, "\0", 1);
        if (extend_res == -1) 
        {           
            close(seg_fd);
            cout<<endl<<"Cannot extend segment";
            return NULL;
        }           
    }

	segment_t s;
	s.segname = string(segname);
	s.size = size_to_create;
	s.address = malloc(size_to_create);
	s.is_valid = 1;
	s.currently_used = 0;
	lseek(seg_fd, 0, 0);     
    if ((extend_res = read(seg_fd, s.address, size_to_create) < 0))
    {
    	cout<<endl<<"Error reading file";
    	close(seg_fd);
    	return NULL;
    }

	rvm->seg_list.push_back(s);
	return s.address;
}

void rvm_unmap(rvm_t rvm, void *segbase)
{
	list<segment_t>::iterator seg_it;
	for(seg_it = rvm->seg_list.begin(); seg_it != rvm->seg_list.end(); seg_it++)
	{
		if(seg_it->address == segbase)
		{
			if(!(seg_it->is_valid))
			{
				cout<<"Segment has not been mapped before";
				return;
			}
			if(seg_it->currently_used)
			{
				cout<<"Segment is in use";
				return;
			}
			break;
		}
	}

	if(seg_it == rvm->seg_list.end())
	{
		cout<<endl<<"Segment does not exist";
		return;
	}

	rvm->seg_list.erase(seg_it);
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
	list<segment_t>::iterator seg_it;
	char rm_seg[200];

	for(seg_it = rvm->seg_list.begin(); seg_it != rvm->seg_list.end(); seg_it++)
	{
		if(seg_it->segname == string(segname))
			break;
	}
	// seg_it = rvm->seg_list.find(string(segname));

	if(seg_it == rvm->seg_list.end())
	{
		cout<<endl<<"Segment does not exist (can't destroy)"<<endl;
		return;
	}

	if(seg_it->is_valid) 
	{
		cout<<endl<<"The segment is currenty mapped (can't destroy)"<<endl;
		return;
	}

	rvm->seg_list.erase(seg_it);

    string seg_path = rvm->file_path + "/" + string(segname);
	string seg_log_path = rvm->file_path + "/" + string(segname) +".log";
	strcpy(rm_seg, "rm ");
    strcat(rm_seg, seg_path.c_str());
    strcat(rm_seg, " ");
    strcat(rm_seg, seg_log_path.c_str());
    system(rm_seg);
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
	list<segment_t>::iterator seg_it;
	transaction_info tid_info;
	tid_info.rvm = rvm;
	int i, flag =0;
	//TODO: use iterator instead of flag.
	for(i = 0; i < numsegs; i++)
	{
		flag = 0;
		for(seg_it = rvm->seg_list.begin(); seg_it != rvm->seg_list.end(); seg_it++)
		{			
			if(seg_it->address == segbases[i])
			{
				flag = 1;
				if (!seg_it->is_valid)
				{
					cout<<"This segment has not been mapped."<<endl;
					return -1;
				}
				if (seg_it->currently_used)
				{
					cout<<"This segment is being used by another transaction"<<endl;
					return -1;
				}
				//<segbase,segment_t*>
				// TODO: can put this into next loop
				tid_info.segments[segbases[i]] = &(*seg_it);
			}
		}
		if(flag == 0)
		{
			cout<<"This segment does not exist"<<endl;
			return -1;
		}
	}
	
	/*
	 * Marking segments to be modified as currenty used
	 */
	map<void*, segment_t*>::iterator it_seg;
	for(it_seg = tid_info.segments.begin(); it_seg != tid_info.segments.end(); it_seg++)
	{
		tid_info.segments[it_seg->first]->currently_used = 1;					
	}

	/*
	 * Adding transaction_info data to global transactions map
	 */
	trans_t tid = global_id++;
	transactions_map[tid]=tid_info;
	return tid;
}

void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
	/*
	 * Error conditions
	 */
	if(transactions_map.count(tid))
	{
		if(transactions_map[tid].segments.count(segbase))
		{
			int seg_size = transactions_map[tid].segments[segbase]->size;
			if(seg_size < offset + size)
			{
				cout<<"Outside the range of the segment"<<endl;
				return;
			}
			undo_log_t undo_log;
			undo_log.offset = offset;
			undo_log.size = size;
			undo_log.backup_data = malloc(size);
			memcpy(undo_log.backup_data, (char*) segbase + offset, size);
			transactions_map[tid].undo_logs[segbase].push_front(undo_log);
		}
		else{
			cout<<"Segment address not part of this transaction"<<endl;
			return;
		}	
	}
	else
	{
		cout<<"TID does not exist"<<endl;
		return;
	}
}

void rvm_commit_trans(trans_t tid)
{
	string directory;
	void *segbase;
	undo_log_t undo_log;
	string log_file_path;
	string segname;
	map<void*, list<undo_log_t> >::iterator undo_it;
	map<void*, segment_t*>::iterator seg_it;
	int log_fd;
	transaction_info tid_info = transactions_map[tid];

	if(transactions_map.count(tid))
	{
		directory = tid_info.rvm->file_path;		
		for(undo_it = tid_info.undo_logs.begin(); undo_it != tid_info.undo_logs.end(); undo_it++)
		{
			segbase = undo_it->first;
			seg_it = tid_info.segments.find(segbase);
			segname = seg_it->second->segname;
			log_file_path = directory + "/" + segname + ".log";
			if((log_fd = open(log_file_path.c_str(), O_RDWR | O_CREAT, 0755)) < 0)
			{
				printf("Error opening log file\n");
				continue;
			}
			lseek(log_fd, 0, SEEK_END);

			while(!undo_it->second.empty())
			{			
				undo_log = undo_it->second.front();
				write(log_fd, &(undo_log.offset), sizeof(int));
				write(log_fd, &(undo_log.size), sizeof(int));
				write(log_fd, (char*) seg_it->second->address + undo_log.offset, undo_log.size);
				undo_it->second.pop_front();
			}
			seg_it->second->currently_used = 0;
			tid_info.segments.erase(seg_it);
			tid_info.undo_logs.erase(undo_it);
			close(log_fd);
		}
		transactions_map.erase(tid);
	}
	else
	{
		cout<<"TID invalid"<<endl;
		return;
	}
}

void rvm_abort_trans(trans_t tid)
{
	transaction_info tid_info = transactions_map[tid];
	map<void*, list<undo_log_t> >::iterator list_it;
	void *segbase;
	undo_log_t undo_log;
	if(transactions_map.count(tid))
	{
		for(list_it = tid_info.undo_logs.begin(); list_it != tid_info.undo_logs.end(); list_it++)
		{
			segbase = list_it->first;
			while(!list_it->second.empty())
			{			
				undo_log = list_it->second.front();			
				memcpy((char*)segbase+undo_log.offset, undo_log.backup_data, undo_log.size);
				list_it->second.pop_front();
			}
			tid_info.undo_logs.erase(list_it);
		}

		map<void*, segment_t*>::iterator seg_it;
		for(seg_it = tid_info.segments.begin(); seg_it != tid_info.segments.end(); seg_it++)
		{
			tid_info.segments[seg_it->first]->currently_used = 0;							
		}
		
		transactions_map.erase(tid);
	}
	else
	{
		cout<<"Invalid transaction id"<<endl;
		return;
	}
}

void rvm_truncate_log(rvm_t rvm)
{
	DIR *d;
    struct dirent *dir;
    string file;

    if(d = opendir(rvm->file_path.c_str()))
    {
	    while ((dir = readdir(d)) != NULL) 
	    {
	        file = string(dir->d_name);
	        if(file.length() > 4 && file.compare(file.length()-4, 4, ".log") == 0)
	        {
	        	cout<<"Log file: "<<file<<endl;
	        	apply_log_for_segment(rvm, file.substr(0, file.length()-4));
	        }
	    }
	    closedir(d);
	}
	else
	{
		cout<<"Could not open directory"<<endl;
	}
}