//All structures used by the RVM are defined here.
#include <map>
#include <string>
#include <cstring>
#include <list>

using namespace std;

typedef int trans_t;

/*
 * Segment attributes
 * is_valid: has been mapped
 * currently_used: in a transaction
 */
typedef struct
{
	char name[256];
	void *address;
	int size;
	int is_valid;
	int currently_used;
} segment_t;

/*
 * RVM
 * Directory name on disk
 * List of segments
 */
typedef struct {
  string file_path;
  //<segname, segment_t>
  map <string, segment_t> segment_map;
} rvm_t;

/*
 * Undo log for specific tid
 * Used within each transaction_info struct
 */
typedef struct 
{
	int offset_val;
	void *backup_data;
	int size;
	int backup_valid;
} undo_log_t;

/*
 * Contains information for each transaction tid
 * RVM, segments modified, undo log
 */
typedef struct 
{
	rvm_t rvm;
	//<segbase (void* address of segment), segment_t*>
	map <void*, segment_t*> segments;
	map <void*, list<undo_log_t> > undo_logs;
} transaction_info;