# Recoverable Virtual Memory
Advanced Operating Systems Assignment 4
--------------------------------------------------------------------------------------------
Name: Nikhil Bharat and Priyanka Ganesan
--------------------------------------------------------------------------------------------
Linux Platform: Ubuntu 14.04
--------------------------------------------------------------------------------------------
Compilation:

Directory Structure:
RVM

-rvm.h

-rvm_internal.h

-rvm.cpp

-README.md

In the top directory(RVM), type: 
$ make
This will give librvm.a

To compile a test case, copy it to the top directory(RVM) and type:
$ g++ -L -lrvm rvm.cpp <test_file> -o <test_name>

--------------------------------------------------------------------------------------------
Virtual Memory Scheme:

Each program which uses the RVM system makes a call to rvm_init(). This initialization function creates a directory on disk which serves as the backing store which holding the virtual memory segments. This can be called multiple times, as long as the backing store location is different. 

Memory Persistence:
The segments stored on disk are described using a unique name, along with the address and size. The address is used in the situations where the memory segment is mapped into main memory. Apart from this, the state of each segment is monitored(whether it is mapped into main memory, and whether it is currently being used in a transaction).


The main APIs which work on transferring the segments between disk and main memory are map(), unmap(), and destroy(). The map() API is called for a particular segment, in order to move the contents between disk and main memory. The segment is checked to see if it is already present, or already mapped. It is then either created or read on disk, the size restrictions are checked, and finally read into a buffer in main memory.


The unmap function is again specific to a segment. It first makes sure that the segment is not being used in a transaction, and that it is actually mapped for it to be unmapped. It then deallocates resources from main memory for that particular segment.


The destroy function removes the entire list of segments from memory as well as from disk, after checking valid conditions. 


Transaction Semantics:
A transaction is a set of operations performed on multiple segments in main memory which are grouped together. In order to avoid losing data to crashes, there needs to way to store past and present data in order to commit data safely back to the disk. To perform this task, we use two log files:


When rvm_begin_transaction is called, a unique transaction ID is created, which is associated with a list of segments associated with the transaction, along with a set of undo logs per segment. 

##Undo log:
This log is created when there is going to be a modification on a segment, and stores a running record of all operations on all segments over the course of the transaction. It contains the offset of the modification, the size, and a backup of the data. It is used in two situations.
* When a running transaction is aborted, this undo log file is read from most to least recent, and the logs are applied to revert to the original state of the segment. This ensures that the data in memory during an abort is consistent with that on disk
* When a transaction is committed, the undo log is converted and appended into multiple redo logs, which are unique to each segment on disk.




##Redo Log:
The redo log is a mapping of the contents of the undo log. It is used in two scenarios:
* When the map() function is called on a segment, the redo log for that particular function is first applied, before mapping happens. This ensures that any uncommited transactions are updated before the segment is mapped.
* When truncate_log() is called, changes from the redo logs are applied into the segments on disk to keep them persistent. Only once all the changes are applied, the redo log is removed from disk and then recreated. This ensures they do not infinitely grow. 

--------------------------------------------------------------------------------------------