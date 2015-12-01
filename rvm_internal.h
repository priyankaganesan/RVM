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
typedef struct segment
{
	string segname;
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
typedef struct rvm
{
	string file_path;
	list<segment_t> seg_list;
} rvm_struct;

typedef rvm_struct* rvm_t;

/*
 * Undo log for specific tid
 * Used within each transaction_info struct
 */
typedef struct 
{
	int offset;
	int size;
	void *backup_data;
	int backup_valid;
} undo_log_t;

/*
 * Undo log for specific tid
 * Used within each transaction_info struct
 */
typedef struct 
{
	int offset;
	int size;
	void *new_data;
} redo_log_t;

/*
 * Contains information for each transaction tid
 * RVM, segments modified, undo log
 */
 typedef struct 
{
	rvm_t rvm;
	map<void*, segment_t*> segments;
	map<void*, list<undo_log_t> > undo_logs;
} transaction_info;