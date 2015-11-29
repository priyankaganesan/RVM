//All structures used by the RVM are defined here.
#include <map>
#include <string>
#include <cstring>
#include <list>

using namespace std;

typedef int trans_t;

typedef struct
{
	char name[256];
	void *address;
	int size;
	int is_valid;
	int currently_used;
} segment_t;

typedef struct {
//  char file_path[256];
  string file_path;
  map <string, segment_t> segment_map;
} rvm_t;

typedef struct 
{
	int offset_val;
	void *backup_data;
	int size;
	int backup_valid;
} undo_log_t;

typedef struct 
{
	rvm_t rvm;
	map <void*, segment_t*> segments;
	map <void*, list<undo_log_t> > undo_logs;
} transaction_info;



