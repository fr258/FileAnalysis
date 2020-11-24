#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>	
#include <pthread.h>
#include <fcntl.h>

#define add(X,Y) nodeAdd(&X, &Y, sizeof(Y))
#define nData(X) ((NameData*)X->data)
#define BUFFSIZE 256

//generic node in linked list
typedef struct listNode
{
    void* data;
    struct listNode *next;
	
} Node;

void initGeneral(Node* node, void* data, int dataSize);
void initNameData(Node* node, char** data);

//node to hold filename and count or token and frequency
typedef struct 
{
	char* name;
	double freq;
	
} NameData;

//standard iterator which works with next() and hasNext()
typedef struct
{
	Node* head;
	
} Iterator;

//arguments which are passed to fileHandler
typedef struct Arguments
{
  char* pathName;
  pthread_mutex_t* mut;
  Node* listHead;
} Args;

//first returns the head node, and after that returns the following node until a pointer to NULL is hit.
//precondition: Iterator passed is valid and not null and the final valid node points to null.
Node* next(Iterator* iter)
{
	if(iter->head != NULL)
	{
		Node* temp = iter->head; //will return current node stored by iterator
		iter->head = iter->head->next; //set iterator to store next node to be returned by next()
		return temp; 
	}
	return NULL;
}

//first returns data of the head node, and after that returns following node's data until a pointer to NULL is hit.
//precondition: Iterator passed is valid and not null and the final valid node points to null.
void* nextData(Iterator* iter)
{
	if(iter->head != NULL)
	{
		Node* temp = iter->head; //will return data of current node stored by iterator
		iter->head = iter->head->next; //set iterator to store next node whose data will be returned by next()
		return temp->data; 
	}
	return NULL;
}
//returns if iterator doesn't point to end of list
//precondition: Iterator passed is valid and not null and the final valid node points to null.
int hasNext(Iterator* iter)
{
	return (iter->head != NULL);
}

//precondition: head isn't null and was initialized to {NULL, NULL}
//adds node to end of list
//returns pointer to newly created node
Node* nodeAdd(Node* head, void* data, int dataSize)
{
	if(head->data != NULL)
	{
		while(head->next != NULL) //travel to end of list
		{
			head = head->next;
		}
		
		head->next = malloc(sizeof(Node)); //create new list
		head = head->next;
	}

	if(dataSize == 3) //if specified, assume data is NameData
	{
		initNameData(head, data);
	}
	else
	{
		initGeneral(head, data, dataSize); //otherwise, copy data to node by bytes
	}
	return head;
}

//inserts node with data type NameData into list alphabetically and returns the head of the list
//assumes that head and data aren't null and that valid node points to null
Node* nodeAddAlpha(Node* head, char** data)
{
	Node* front = head; 
	if(nData(head)!=NULL)
	{
		Node *temp;
		int compare = strcmp(*data, nData(head)->name);
		if(compare<0) //data is alphabetically smaller than head of list
		{
			front = malloc(sizeof(Node)); //set new data to front of list
			initNameData(front, data);
			front->next = head;
		}
		else if(compare == 0)
		{
			nData(head)->freq++;
		}
		else
		{
			while(head->next != NULL && strcmp(*data, nData(head->next)->name)>=0)
			{
				head = head->next; //increment to correct position alphabetically
			}
			
			compare = strcmp(*data, nData(head)->name);
			
			if(compare != 0) //token isn't in list, so insert new token
			{
				temp = malloc(sizeof(Node));
				initNameData(temp, data);
				
				temp->next = head->next;
				head->next = temp;	
			}
			else if(compare == 0) //token already exists in list
			{
				nData(head)->freq++; //increase count of preexisting token
			}
		}
	}
	else
	{
		initNameData(head, data); //set new data as first node of list
	}
	return front; //return head of list
}

//copies dataSize number of bytes into data of passed node
//assumes passed node and data aren't null
void initGeneral(Node* node, void* data, int dataSize)
{
	node->data = malloc(dataSize);
	memcpy(node->data, data, dataSize);
	node->next = NULL;
}

//copies string by value into passed node
//sets default of count of tokens to 1
//assumes passed node and data aren't null
void initNameData(Node* node, char** data)
{
	node->data = malloc(sizeof(NameData));
	NameData* temp = nData(node);
	temp->name = strdup(*data);
	temp->freq = 1.0f; 
	node->next = NULL;
}

//frees data allocated in list
//assumes head isn't null and dataSize matches size of data in nodes
//assumes each node of list has same size
//doesn't free first node, which might not have been malloced
void deleteList(Node* head, size_t dataSize)
{
	Iterator iter = {head}, iter1 = {head};
	if(dataSize == 3) //list is of NameData nodes
	{
		NameData* temp;
		while(hasNext(&iter))
		{
			temp = nData(next(&iter));
			free(temp->name); //free stored string
			free(temp); //free data
		}
	}
	else //list is of general nodes
	{
		while(hasNext(&iter))
		{
			free(nextData(&iter)); //free data
		}
	}
	next(&iter1); //move past first node, which might not have been malloced
	while(hasNext(&iter1))
	{
		free(next(&iter1)); //free node
	}
}

//increase file node's count of total tokens
void addToken(Node* node, char** data)
{
	nData(node)->freq++; //increase file node's count of total tokens
	if(node->next == NULL) //no tokens in list, so input is first token
	{
		node->next = malloc(sizeof(Node)); 
		node->next->next = NULL;
		node->next->data = NULL;
	}
	node->next = nodeAddAlpha(node->next, data); //insert token alphabetically
}

//called by tokenizer, removes invalid characters and sets to lowercase
//assumes node and name are valid and aren't null and length is length is length of name
void tokenHelper(Node* node, char* name, int length)
{
	char temp[length+1]; //+1 for null terminator
	int i = 0, b = 0;
	for(i = 0, b = 0; i<length; i++) //check each token in list to see if valid
	{
		if(isalpha(*(name+i)))
		{
			temp[b] = tolower(*(name+i)); //make letter lowercase
			b++;
		}
		else if(*(name+i) == '-')
		{
			temp[b] = *(name+i);
			b++;
		}
	}
	temp[b] = '\0'; //make string null-terminated
	if(b>0) //token contains at least 1 valid character
	{
		char* temp2 = temp;
		addToken(node, &temp2); //add token to list
	}

}
//called by tokenizer, sends tokens to be added to list or have incomplete token be stored
//precondition: there is no incomplete token being currently stored
void tokenHelperTwo(Node* node, char* token, char* buffer, char** temp1, int length, int* isCont)
{
	if(token-buffer+length < BUFFSIZE)
	{
		tokenHelper(node,token, strlen(token)); //entire token is in buffer, so have be added to list
		*isCont = 0; //mark that no incomplete is being stored
	}
	else
	{
		*temp1 = strdup(token); //store incomplete token
		*isCont = 1; //mark that incomplete token is being stored
	}
}

//reads through file and breaks text into tokens to be added to list
//assumes that all text in file is ascii
//assumes that file descriptor is valid

void tokenizer(Node* node, int fd)
{
	int bytes; //number of bytes read in
	int length; //length of token string
	char buffer[BUFFSIZE+1]; //buffer plus a null terminator
	int inBuffer; //marks whether or not at least one nondelimter was in buffer
	char* temp1 = NULL, *temp2 = NULL;
	int isCont = 0; //marks whether or not incomplete token is being stored
	buffer[BUFFSIZE] = '\0'; //end buffer with terminating null 
	
	char* delim = "\n "; //split tokens by newlines and spaces
	char* token;
	
	bytes =  read(fd, buffer, BUFFSIZE);
	token = strtok(buffer, delim);
	while(bytes>0) //read file while there are still bytes in it
	{
		inBuffer = 0; //currently 0 nondelimters in buffer
		while(token != NULL)
		{
			inBuffer = 1; //at least 1 nondelimiter in buffer
			length = strlen(token);
			
			if(isCont == 1) //there is incomplete token being stored
			{			
				if(buffer[0]==' ' || buffer[0]=='\n') //stored token is actually complete token
				{
					tokenHelper(node, temp1, strlen(temp1)); //remove invalid characters and add to list
					free(temp1); //release stored token
					tokenHelperTwo(node, token,buffer,&temp1,length,&isCont); //store or add current token
				}
				else //stored token is incomplete
				{
					temp2 = malloc(length + strlen(temp1)+1);  //add current token to stored token
					strcpy(temp2, temp1);
					strcat(temp2, token);
					free(temp1);
					temp1 = temp2;
					if(token-buffer+length < BUFFSIZE) //current token is complete
					{
						tokenHelper(node, temp1, strlen(temp1)); //process and add token to list
						free(temp1); //release stored token
						isCont = 0; //there is no stored token
					}
				}
			}
			else //nothing stored
			{			
				tokenHelperTwo(node,token,buffer,&temp1,length,&isCont); //store or add current token			
			}			
			token = strtok(NULL, delim);
			
		} //end while token
		
		bytes =  read(fd, buffer, BUFFSIZE);
		buffer[bytes] = '\0'; //null terminate read-in bytes
		token = strtok(buffer, delim);
		if(inBuffer == 0 && isCont == 1) //only delimeters in buffer, stored token is complete
		{
			tokenHelper(node, temp1, strlen(temp1)); //proccess and add stored token to list
			free(temp1); //release stored token
			isCont = 0; //there is no stored token
			
		}
		
	} //end while bytes
	
	if(isCont==1 && temp1!=NULL) //there is stored token at end, is therefore complete token
	{
		tokenHelper(node, temp1, strlen(temp1)); //proccess and add stored token to list
		free(temp1); //release stored token
	}
}

//attempts to open passed filename and has file tokenized if successful
//if cannot open file, prints error message
//assumes that it is passed Args struct with nonnull params
void* fileHandler(void* input)
{
	Args args = *(Args*)input;
	int fd = open(args.pathName, O_RDONLY);
	
	if(fd<0) //passed filename was invalid
	{
		perror(args.pathName); 
	}
	else
	{
		Node* temp = malloc(sizeof(Node));
		temp->next = NULL;
		temp->data = NULL;	
		
		
		pthread_mutex_lock(args.mut); //lock main list while node is added to it
		
		Node* temp2 = (Node*)(nodeAdd(args.listHead, temp, sizeof(*temp))->data); //add linked list to list

		pthread_mutex_unlock(args.mut); //release lock when node has been added
		
		Node* tokenHead = nodeAdd(temp2, &args.pathName, 3); //add file node to new linked list
		nData(tokenHead)->freq--; //set file token count to 0
		tokenizer(tokenHead, fd); //tokenize file


		int count = (int)nData(tokenHead)->freq;
		Iterator iter = {tokenHead};
		double* freq;
		next(&iter);
		while(hasNext(&iter)) //divide every token count by number of tokens
		{
			freq = &nData(next(&iter))->freq;
			*freq = *freq / count;
		}
		
		close(fd);
		free(temp);
		return tokenHead;
	}
	close(fd);
	return NULL;
}

int findTokenName(Node* head, char* name)
{
	Iterator iter = {head->next};
	Node* temp;
	int compare;
	while(hasNext(&iter))
	{
		temp = (next(&iter));
		compare = strcmp(name, nData(temp)->name);
		if(compare==0)
		{
			return 1;
		}
		else if(compare<0)
		{
			return 0;
		}
		
	}
	return 0;
}

char* pathGenerator(char* path, char* name)
{
    int p = strlen(path);
    int n = strlen(name);
    char* newPath = malloc((p+n+1)*sizeof(char));
    for(int i = 0; i<p; i++)
    {
        newPath[i] = path[i];
    }
    newPath[p] = '/';
    for(int j = p+1; j<(n+p+1); j++)
    {
        newPath[j] = name[j-(p+1)];
    }
    return newPath;
}

void* directoryHandler(void* in)
{
    //cast input to struct Arguments, can extract data such as filepath, mutex, main linkedlist
    Args *args = (Args*)in;
    //checks if directory is accessible, if not returns an error
    DIR *d = opendir(args->pathName);
    if(!d)
    {
        printf("There was an error with the directory at: %s", args->pathName);
        return 0;
    }
    //create list of threads for each item unearthed in this directory
    Node *threads = malloc(sizeof(Node));
    threads->data = NULL;
    threads->next = NULL;
    //store pointer to the head for later
    Node *head = threads;

    //iterate through every item in the directory. Two cases: file or directory
    struct dirent *dp;
    while(dp = readdir(d))
    {
        if(dp->d_type==DT_DIR)
        {
            //Create thread with directoryHandler function for the directory found.
            //Pass along the mutex, the main linked list, and the updated path, in a new struct
            //Adds this thread to the linkedlist of threads associated with this particular function call
            Args *a1 = malloc(sizeof(Args));
            a1->listHead = args->listHead;
            a1->mut = args->mut;
            a1->pathName = pathGenerator(args->pathName, dp->d_name);
            pthread_t t1;
            pthread_create(&t1, NULL, &directoryHandler, a1);
            nodeAdd(threads, &t1, sizeof(pthread_t));
            threads->next = malloc(sizeof(Node));
            threads = threads->next;
        }
        else
        {
            //Create thread with fileHandler function for the directory found.
            //Pass along the mutex, the main linked list, and the updated path, in a new struct
            //Adds this thread to the linkedlist of threads associated with this particular function call
            Args *a1 = malloc(sizeof(struct Arguments));
            a1->listHead = args->listHead;
            a1->mut = args->mut;
            a1->pathName = pathGenerator(args->pathName, dp->d_name);
            pthread_t t1;
            pthread_create(&t1, NULL, &fileHandler, a1);
            nodeAdd(threads, &t1, sizeof(pthread_t));
            threads->next = malloc(sizeof(struct listNode));
            threads = threads->next;
        }
    }
    //go through list and join threads
    while(head!=NULL)
    {
        pthread_join((pthread_t)(head->data), NULL);
        head = head->next;
    }
    closedir(d);
}

int main(int argc, char *argv[])
{
    //call a thread with the following arguments for the directory handler funciton:
    Args *a = malloc(sizeof(Args));
    Node *bigList = malloc(sizeof(Node));
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    a->pathName = argv[1];
    a->listHead = bigList;
    a->mut = &lock;

    pthread_t t;
    pthread_create(&t, NULL, &directoryHandler, a);

    //MATH ANALYSIS
}