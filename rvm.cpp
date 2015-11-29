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

using namespace std;

string LOGFILE = "rvm.log";
map<string, segment_t>::iterator segment_it;

rvm_t rvm_init(const char *directory)
{
	rvm_t rvm;
	char file[256];
	char dir_str[256];
	struct stat* sbuf;
	rvm.file_path=string(directory);
	if (stat(directory, sbuf) == 0 && S_ISDIR(sbuf->st_mode))
	{
		//May need to change this later.
	    cout<<endl<<"Directory already exists";
	    return rvm;	    
	}
	strcpy (dir_str, "mkdir ");
	strcat (dir_str, directory);
	system(dir_str);
	return rvm;
}

//Read operation could be performed after the write to see if it is successful.

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
	int len=rvm.file_path.length() +strlen(segname) + 1;
	int fp;
	int flag;
	int file_size;
	char complete_path[256];
	char logpath[256];
	segment_it = rvm.segment_map.find(segname);
	if(segment_it != rvm.segment_map.end())
	{
		if(segment_it->second.is_valid == 1)
		{
			cout<<endl<<"Segment already mapped";
			return NULL;
		}
	}
//	open ("test.txt", std::fstream::in | std::fstream::out | std::fstream::app);

	strcpy(complete_path, rvm.file_path.c_str());
	strcat(complete_path, "/");
	strcat(complete_path, segname);
	fp=open(complete_path, O_RDWR | O_CREAT, 0755); //readable and writable with the execute bit set.
	if(fp==-1)
	{
		cout<<endl<<"Error while opening file";
		return NULL;;
	}

	file_size = lseek(fp, 0, SEEK_END); //The offset is set to the end of the file.

	if(file_size<0)
	{
		cout<<endl<<"Error";
		close(file_size);
		return NULL;
	}
	//The segment must be expanded if it is smaller than the size to create.
	if (file_size < size_to_create)						
    {
        lseek(fp, size_to_create - 1, SEEK_SET); //The offset is set to the specified size_to_create.
        flag = write(fp, "\0", 1);
        if (flag == -1) {           
            cout<<endl<<"Error";
            close(fp);
            return NULL;
        }           
    }

    segment_t new_segment;
//    new_segment=(segment_t*)malloc(sizeof(segment_t));
    strcpy(new_segment.name,segname);
    new_segment.size=size_to_create;
    new_segment.is_valid=1;
    new_segment.currently_used=0;
    new_segment.address=(char*) malloc(sizeof(char)*size_to_create);

    rvm.segment_map[string(segname)] = new_segment;
    close(fp);
    return rvm.segment_map[string(segname)].address;
}

void rvm_unmap(rvm_t rvm, void *segbase)
{
	//Find the location of the mapped segment and check if the segbase is same as the address on hashmap.
	for(segment_it = rvm.segment_map.begin(); segment_it != rvm.segment_map.end(); segment_it++)
	{
		if(segment_it->second.address == segbase)
		{
			break;
		}
	}
	//Check if mapping exists
	if(segment_it == rvm.segment_map.end())
	{
		cout<<endl<<"Segment does not exist";
		return;
	}

	//Check if the segment hasn't been mapped
	if(segment_it->second.is_valid == 0)
	{
		cout<<endl<<"No mapping exists";
		return;
	}

	//Check if the segment is being modified in a transaction
	if(segment_it->second.currently_used)
	{
		cout<<endl<<"This segment is busy";
		return;
	}

	free(segbase);
	rvm.segment_map.erase(segment_it);
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
	char command[256];
	segment_it = rvm.segment_map.find(segname);

	if(segment_it->second.is_valid == 1)			 
	{
		cout<<endl<<"The segment is currently mapped";
		return;
	}
	free(segment_it->second.address);
	rvm.segment_map.erase(segment_it);
	strcpy(command, "rm ");
	strcat(command, rvm.file_path.c_str());
	strcat(command, "/");
	strcat(command, segname);
	system(command);
	return;
}

int main(int argc, char** argv)
{
	return 0;
}


