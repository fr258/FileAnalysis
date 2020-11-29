#include <stdio.h>
#include <math.h>
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

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34;1m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"
 
//general node for linked list
typedef struct listNode
{
    void* data;
    struct listNode *next;
	
} Node;

//node data for linked list
typedef struct 
{
	char* name;
	double freq;
	
} NameData;

void initGeneral(Node* node, void* data, int dataSize);
void initNameData(Node* node, void* data, int temp);

//node data for jensen-shannon distance
typedef struct outputStructType
{
    char* name1;
    char* name2;
    double distance;
    int count;
} outputStruct;

//returns a linked list iterator
//head must not be null
typedef struct
{
	Node* head;
	
} Iterator;

//arguments for file and directory threads
//pathName, mutex pointer, and node must not be null
typedef struct Arguments
{
  char* pathName;
  pthread_mutex_t* mut;
  Node** listHead;
} Args;

//returns the next value in a linked list
//iterator passed must not be null
Node* next(Iterator* iter)
{
	if(iter->head != NULL)
	{
		Node* temp = iter->head; //save current head
		iter->head = iter->head->next; //set head in iterator to next node
		return temp; //return current head
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

//returns if at lesat one more node with data exists in list
//iterator must not be null
int hasNext(Iterator* iter)
{
	return (iter->head != NULL);
}

//precondition: head isn't null and was initialized to {NULL, NULL}
//adds node to back of current list
Node* nodeAdd(Node* head, void* data, int dataSize)
{
	if(head->data != NULL) //at least one node exists in list
	{
		while(head->next != NULL)
		{
			head = head->next;
		}
		
		head->next = malloc(sizeof(Node));
		head = head->next;
		
	}
	if(dataSize == 3) //node data is of type NameData
	{
		initNameData(head, data, 0);
	}
	else //node data type is unknown, will be copied by bytes
	{
		initGeneral(head, data, dataSize);
	}
	return head;
}

//expects two NameData structs that are not null
//returns difference between names stored in both
int compareStr(void* one, void* two)
{
	return strcmp(((NameData*)one)->name, ((NameData*)two)->name);
}

//expects two nodes that are not null
//returns difference between frequencies stored in both
int compareCountNameData(void* one, void* two)
{	
	Node* nOne = (Node*) one;
	Node* nTwo = (Node*) two;
	return nData(nOne)->freq - nData(nTwo)->freq;
}

//expects two OutputNode structs that are not null
//returns difference between counts stored in both
int compareCountOutput(void* one, void* two)
{
	return ((outputStruct*)one)->count - ((outputStruct*)two)->count;
}

//expects head to be initialized to NULL, NULL and for head to not be null
//adds node in order in list, relying on key to tell what comparison to use
// 0 says to compare node frequencies, with node data of type NameData
// 1 says to compare node names, with node data of type NameData
// 2 says to compare node counts, with node data of type OutputNode
//returns head of list
Node* nodeAddSort(Node* head, void* data, int key)
{
	Node* front = head; 
	int (*compare) (void*, void*);
	void (*init) (Node*, void*, int);
	int size = 0;
	
	if(key == 0) //passed a list of linked lists-- compare head nodes' token counts
	{
		compare = compareCountNameData;
		size = sizeof(Node);
		init = initGeneral;
	}
	else if(key == 1) //passed a linked list with data type NameData-- compare node names
	{
		compare = compareStr;
		init = initNameData;
	}
	else if(key == 2) //passed a linked list with data type OutputNode-- compare node counts
	{
		compare = compareCountOutput;
		init = initGeneral;
		size = sizeof(outputStruct);
	}

	if(head->data!=NULL) //at least one node already in list
	{
		Node *temp;
		int diff = compare(data, head->data);
		if(diff<0) //passed data is smaller than head node
		{
			front = malloc(sizeof(Node));
			init(front, data, size);
			front->next = head; //set passed data to new front node
		}
		else if(diff == 0 && key == 1) //passed token equals current token
		{
			nData(head)->freq++; //increment frequency of token
		}
		else
		{
			while(head->next != NULL && compare(data, head->next->data) >= 0) //travel through list while passed data is larger than current data
			{
				head = head->next;
			}
			
			diff = compare(data, head->data);
			
			if(diff == 0 && key==1) //passed token equals current token
			{
				nData(head)->freq++; //increment frequency of token
			}
			else //add token to list 
			{
				temp = malloc(sizeof(Node));
				init(temp, data, size);
				
				temp->next = head->next;
				head->next = temp;	
			}

		}
	}
	else //passed data will be first node in list 
	{
		init(head, data, size);
	}
	return front; //return head of list
}


//initialize data by copying bytes
//expects node to not be null
//expects dataSize to be size of what data points to
void initGeneral(Node* node, void* data, int dataSize)
{
    node->data = malloc(dataSize);
    memcpy(node->data, data, dataSize);
    node->next = NULL;
}

//initialize data of type NameData
//expects node to not be null
//placeholder results in same function args as initGeneral for function pointer compatibility
void initNameData(Node* node, void* in, int placeholder)
{
	NameData* data = (NameData*) in;
	node->data = malloc(sizeof(NameData));
	NameData* temp = nData(node);
	temp->name = strdup(data->name);
	temp->freq = data->freq; 
	node->next = NULL;
}

//deletes data and nodes of passed list
//assumes that head is not null, and that dataSize is size of data in node if not NameData nod
//if NameData node, dataSize is 3
void deleteList(Node* head, size_t dataSize)
{
	Iterator iter = {head}, iter1 = {head};
	if(dataSize == 3) //NameData-- must delete malloced string as well as data
	{
		NameData* temp;
		while(hasNext(&iter))
		{
			temp = nData(next(&iter));
			free(temp->name);
			free(temp);
		}
	}
	else //not NameData, will only free data bytes
	{
		while(hasNext(&iter))
		{
			free(nextData(&iter));
		}
	}
	next(&iter1);
	while(hasNext(&iter1)) //free nodes
	{
		free(next(&iter1));
	}
}

//adds token to linked lsit by name alphabetically
//assumes that node is not null and contains file
void addToken(Node* node, NameData* data) 
{
	nData(node)->freq++; //increase count of token
	if(node->next == NULL)
	{
		node->next = malloc(sizeof(Node));
		node->next->next = NULL;
		node->next->data = NULL;
	}
	node->next = nodeAddSort(node->next, data, 1); //adss to list alphabetically
}

//called by tokenizer
//assumes node is not null
//removes non-alphabet characters, excluding hyphens, and makes alphabet characters lowercase
void tokenHelper(Node* node, char* name, int length)
{
	char temp[length+1]; //+1 for null terminator
	int i = 0, b = 0;
	for(i = 0, b = 0; i<length; i++)
	{
		if(isalpha(*(name+i))) //is letter
		{
			temp[b] = tolower(*(name+i)); //make letter lowercase and add to token
			b++;
		}
		else if(*(name+i) == '-') //is letter
		{
			temp[b] = *(name+i); //add character to token
			b++;
		}
	}
	temp[b] = '\0';
	if(b>0) //token contains at least 1 valid character
	{
		NameData data = {temp, 1};
		addToken(node, &data); //add token to linked list 
	}

}

//called by tokenizer
//assumes that none of passed args are null
void tokenHelperTwo(Node* node, char* token, char* buffer, char** incompleteToken, int length, int* isCont)
{
	if(token-buffer+length < BUFFSIZE) //token is fully contained in buffer
	{
		tokenHelper(node,token, strlen(token));
		*isCont = 0; //mark that token isn't stored
	}
	else //token is not fully contained in buffer
	{
		*incompleteToken = strdup(token); //store incomplete token 
		*isCont = 1; //mark that token is stored
	}
}

//called by tokenizer
//seaches for delimters and makes them null characters
//returns pointer to first not-null character
 char* my_strtok(char** currStr)
{
	int length = strlen(*currStr);
	int found = 0;
	int i = 0;
	
	for(i = 0; i<length && !found; i++) //increments first character until nonterminator is found
	{		
		if((*currStr)[i] != ' ' && (*currStr)[i] != '\n')
		{
			found = 1; //mark nonterminator found
			*(currStr) = *(currStr) + i; //set beginning of string to first nonterminator character
		}
	}	
	if(found != 0) //at least one nonterminator character was found
	{
		found = 0;
		for(i = 0; i<length && !found; i++)
		{
			if((*currStr)[i]==' ' || (*currStr)[i]=='\n') //if terminator found
			{
				(*currStr)[i]='\0'; //set terminating character to null
				found = 1;//mark terminator found-- cease looping
			}
		}	
		char* temp = *currStr;
		*currStr = *(currStr)+i;
		return temp; //return pointer to first not-null character
	}
	return NULL; //no nonterminator characters found
}

//assumes not null node and valid file descriptor
//reads file into buffer and splits read file into tokens, which are added to passed linked list
//if file is empty, returns 0
int tokenizer(Node* node, int fd)
{
	int bytes;
	int length;
	char* buffer = malloc(BUFFSIZE+1);
	char* head = buffer; 
	int inBuffer; //used to mark if at least one nonterminator was found
	char* temp1 = NULL, *temp2 = NULL; //used to store incomplete tokens
	int isCont = 0; //is continued from previous token?
	buffer[BUFFSIZE] = '\0'; //buffer null terminated so can be read as string
	char* token;
	int returnVal = 0; //if 0 at end, file is empty
	bytes =  read(fd, head, BUFFSIZE);
	token = my_strtok(&buffer); //return first token of buffer

	while(bytes>0)
	{
		inBuffer = 0;
		while(token != NULL)
		{
			inBuffer = 1;
			length = strlen(token);
			
			if(isCont == 1) //incomplete token being stored
			{			
				if(head[0]==' ' || head[0]=='\n') //token being stored can be added to list
				{
					tokenHelper(node, temp1, strlen(temp1)); //add to list
					free(temp1); //release stored token
					tokenHelperTwo(node, token, head, &temp1, length, &isCont); //process current buffer
				}
				else //token being stored cannot be added to list yet
				{
					//add current token to stored incomplete token
					temp2 = malloc(length + strlen(temp1)+1);
					strcpy(temp2, temp1);
					strcat(temp2, token);
					free(temp1);
					temp1 = temp2;
					if(token-head+length < BUFFSIZE) //if stored token is complete
					{
						tokenHelper(node, temp1, strlen(temp1)); //add to list
						free(temp1); //free stored token
						isCont = 0; //mark that token isn't being stored
					}
				}
			}
			else //incomplete token not being stored
			{			
				tokenHelperTwo(node,token,head,&temp1,length,&isCont);				
			}	
		
			token = my_strtok(&buffer); //get new token from buffer

			
		} //end while token
		
		bytes =  read(fd, head, BUFFSIZE); //read new bytes into buffer
		buffer = head; //set pointer to beginning of buffer
		buffer[bytes] = '\0'; //terminate buffer with null character

		token = my_strtok(&buffer); //get new token from buffer

		if(inBuffer == 0 && isCont == 1) //stored token, buffer has no valid characters
		{
			tokenHelper(node, temp1, strlen(temp1)); //add stored token to list
			free(temp1); //free stored token
			isCont = 0; //mark that no token is being stored
			
		}
		
		returnVal = 1; //mark that file isn't empty
		
	} //end while bytes
	
	if(isCont==1 && temp1!=NULL) //stored token but reached end of file
	{
		tokenHelper(node, temp1, strlen(temp1)); //add token to list
		free(temp1); //release stored token
	}
	free(head);
	return returnVal;
}

//reads file, has it tokenized, and has it added to list by number of tokens
void* fileHandler(void* input)
{
	Args args = *(Args*)input;
	int fd = open(args.pathName, O_RDONLY);

	if(fd<0) //invalid pathName passed
	{
		perror(args.pathName);
	}	
	else //valid pathName passed
	{

		Node* file = malloc(sizeof(Node)); //create new head node for file
		file->next = NULL;
		file->data = malloc(sizeof(NameData));
		nData(file)->freq = 0;
		nData(file)->name = strdup(args.pathName);
		
		int val = tokenizer(file, fd); //tokenize file and add token nodes after file node
		
		if(val) //file isn't empty
		{
			int count = (int)nData(file)->freq;
			Iterator iter = {file};
			double* freq;
			next(&iter);
			while(hasNext(&iter)) //divide token counts by total number of tokens
			{
				freq = &nData(next(&iter))->freq;
				*freq = *freq / count; 
			}
			
			pthread_mutex_lock(args.mut);
			*(args.listHead) = nodeAddSort(*(args.listHead), file, 0); //add node by token count
			pthread_mutex_unlock(args.mut);
		}
		else //file is empty, free resources
		{
			free(nData(file)->name);
			free(nData(file));
		}
		
		free(file);
	}
	close(fd);
	return NULL;
}

double findTokenName(Node* head, char* name)
{
	Node* temp = head;
	while(temp!=NULL)
	{
		if(strcmp(name, nData(temp)->name)==0)
		{
			return nData(temp)->freq;
		}
		temp = temp->next;
	}
	return 0;
}

char* pathGenerator(char* path, char* name)
{
    int p = strlen(path);
    int n = strlen(name);
    char* newPath = malloc(p+n+2);
	strcpy(newPath, path);
	strcat(newPath, "/");
	strcat(newPath, name);
	
    return newPath;
}

void* directoryHandler(void* in)
{
	int notEmpty = 0;
	//printf("in directory handler!\n");
    //cast input to struct Arguments, can extract data such as filepath, mutex, main linkedlist
    Args *args = (Args*)in;
    //checks if directory is accessible, if not returns an error
	DIR *d;

    d = opendir(args->pathName);

    if(d==NULL)
    {
        printf("There was an error with the directory at: %s\n", args->pathName);
        return NULL;
    }

    //create list of threads for each item unearthed in this directory
    Node threads = {NULL, NULL};
	//store args structs passed to threads to be freed at end
	Node nodeArgs = {NULL, NULL};
	//store each args to be passed to nodeArgs
	Node* temp;
 

    //iterate through every item in the directory. Two cases: file or directory
    struct dirent *dp;
	dp = readdir(d);
	dp = readdir(d);
    while((dp = readdir(d)) != NULL)
    {
        if(dp->d_type==DT_DIR)
        {
            //Create thread with directoryHandler function for the directory found.
            //Pass along the mutex, the main linked list, and the updated path, in a new struct
            //Adds this thread to the linkedlist of threads associated with this particular function call

				Args a1 = {pathGenerator(args->pathName, dp->d_name), args->mut, args->listHead};
				pthread_t t1;
				notEmpty = 1;
				temp = nodeAdd(&nodeArgs, &a1, sizeof(Args));
				pthread_create(&t1, NULL, &directoryHandler, temp->data);
				add(threads, t1);

        }
        else if(dp->d_type==DT_REG)
        {
            //Create thread with fileHandler function for the directory found.
            //Pass along the mutex, the main linked list, and the updated path, in a new struct
            //Adds this thread to the linkedlist of threads associated with this particular function call
			Args a1 = {pathGenerator(args->pathName, dp->d_name), args->mut, args->listHead};
			pthread_t t1;
			notEmpty = 1;
			temp = nodeAdd(&nodeArgs, &a1, sizeof(Args));
            pthread_create(&t1, NULL, &fileHandler, temp->data);
			add(threads, t1);
        }
		else
		{
			printf("invalid\n");
		}

    }
    //go through list and join threads
	if(notEmpty)
	{
		Iterator iter = {&threads};
		while(hasNext(&iter))
		{
			pthread_join(*(pthread_t*)nextData(&iter), NULL);
		}
		iter.head = &nodeArgs;
		while(hasNext(&iter)) //release malloced args
		{
			Node* temp = next(&iter);
			free(((Args*)(temp->data))->pathName);
		}
	}

    closedir(d);
	deleteList(&threads, 0);
	deleteList(&nodeArgs, 0);
	return NULL;
}

int listLength(Node* in)
{
    Node* list = (Node*)in->data;
    int count = 0;
    while(list!=NULL)
    {
        count++;
        list = list->next;
    }
    return count;
}

double analyzePair(Node* in1, Node* in2)
{
	Node* meanConstruct = malloc(sizeof(Node));
	meanConstruct->data = NULL;
	meanConstruct->next = NULL;
	Node* token1 = in1->next;
	Node* token2 = in2->next;
	while(token1!=NULL)
	{
		NameData* data1 = (NameData*)token1->data;
		double freq1 = data1->freq;
		double freq2 = findTokenName(token2, data1->name);
		
		NameData construct = {data1->name, (freq1+freq2)/2};
		nodeAdd(meanConstruct, &construct, 3);
		
		token1 = token1->next;
	}
	token1 = in1->next;
	while(token2!=NULL)
	{
		NameData* data2 = (NameData*)token2->data;
		double freq2 = data2->freq;
		double freq1 = findTokenName(token1, data2->name);
		//printf("GENERIC TOKEN: %s\n", data2->name);
		if(freq1 == 0)
		{
			//printf("NON DUPLICATE TOKEN: %s\n", data2->name);
			NameData construct = {data2->name, (freq2)/2};
			nodeAdd(meanConstruct, &construct, 3);
		}
		token2 = token2->next;
	}
	/*Node* meanConstructTestPointer = meanConstruct;
	while(meanConstructTestPointer!=NULL)
	{
		printf("Token name, mean freq: %s %f\n", ((NameData*)meanConstructTestPointer->data)->name, ((NameData*)meanConstructTestPointer->data)->freq);
		meanConstructTestPointer = meanConstructTestPointer->next;
	}*/

	double kld1 = 0;
	double kld2 = 0;
	token1 = in1->next;
	while(token1!=NULL)
	{
		NameData* data1 = (NameData*)token1->data;
		double freq1 = data1->freq;
		double mean1 = findTokenName(meanConstruct, data1->name);
		double loggy = log10(freq1/mean1);
		kld1 += (freq1 * loggy);
		token1 = token1->next;
	}
	/*printf("kld1: %f\n", kld1);
	char* test = malloc(sizeof(char));
	*test = 'a';
	printf("frequency of a in meanConstruct: %f\n", findTokenName(meanConstruct, test));*/


	
	token2 = in2->next;
	while(token2!=NULL)
	{	
		NameData* data2 = (NameData*)token2->data;
		double freq2 = data2->freq;
		//printf("freq2: %f\n", freq2);
		double mean2 = findTokenName(meanConstruct, data2->name);
		//printf("name: %s\n", data2->name);
		//printf("mean2: %f\n", mean2);
		double loggy = log10(freq2/mean2);
		//printf("loggy: %f\n", loggy);
		kld1 += (freq2 * loggy);
		token2 = token2->next;
	}
	//printf("kld2: %f\n", kld1);
	
	deleteList(meanConstruct, 3);
	free(meanConstruct);
	return (kld1+kld2)/2;
}

void analyze(Node* in)
{
	Node* start = in;
    Node* outputList = malloc(sizeof(Node));
    outputList->data = NULL;
    outputList->next = NULL;
	while(start!=NULL && start->next!=NULL)
	{
		Node* current = start->next;
		while(current!=NULL)
		{
			Node* p1 = (Node*)start->data;
			Node* p2 = (Node*)current->data;
			double result = analyzePair(p1, p2);
            outputStruct out = {((NameData*)p1->data)->name, ((NameData*)p2->data)->name, result, (((NameData*)p1->data)->freq + ((NameData*)p2->data)->freq)};
            outputList = nodeAddSort(outputList, &out, 2);

			current = current->next;
		}
		start = start->next;
	}
    Node* outputCurrent = outputList;
    
    while(outputCurrent!=NULL && outputCurrent->data!=NULL)
    {
        double result = ((outputStruct*)(outputCurrent->data))->distance;
        char* name1 = ((outputStruct*)(outputCurrent->data))->name1;
        char* name2 = ((outputStruct*)(outputCurrent->data))->name2;
        if(result<=0.1)
        {
            printf(ANSI_COLOR_RED     "%f"     ANSI_COLOR_RESET "", result);
        }
        else if(result<=0.15)
        {
            printf(ANSI_COLOR_YELLOW    "%f"     ANSI_COLOR_RESET "", result);
        }
        else if(result<=0.2)
        {
            printf(ANSI_COLOR_GREEN    "%f"     ANSI_COLOR_RESET "", result);
        }
        else if(result<=0.25)
        {
            printf(ANSI_COLOR_CYAN    "%f"     ANSI_COLOR_RESET "", result);
        }
        else if(result<=0.3)
        {
            printf(ANSI_COLOR_BLUE    "%f"     ANSI_COLOR_RESET "", result);
        }
        else if(result>0.3)
        {
            printf(ANSI_COLOR_WHITE   "%f"     ANSI_COLOR_RESET "", result);
        }
        printf(" \"%s\" and \"%s\" \n", name1, name2);
        outputCurrent = outputCurrent->next;
    }
	deleteList(outputList,0);
	free(outputList);
}

void printTest(Node* in)
{
    Node* current = in;
    while(current!=NULL && current->data != NULL)
    {
        Node* tokens = (Node*)current->data;
        NameData* info = (NameData*)tokens->data;
        printf("File name: %s\n", info->name);
        printf("Word count: %f\n", info->freq);
		tokens = tokens->next;
		while(tokens!=NULL)
		{
			NameData* info = (NameData*)tokens->data;
			printf("Token: %s\n", info->name);
       		printf("Frequency: %f\n", info->freq);
			tokens = tokens->next;
		}
		printf("\n");
        current = current->next;
    }
}

int main(int argc, char *argv[])
{
    //call a thread with the following arguments for the directory handler funciton:
	if(argc<2)
	{
		printf("No directory passed\n");
		return EXIT_SUCCESS;
	}
    Args *a = malloc(sizeof(Args));
    Node *bigList = malloc(sizeof(Node));
    bigList->data = NULL;
    bigList->next = NULL;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    a->pathName = argv[1];  
    a->listHead = &bigList;
    a->mut = &lock;
    pthread_t t;

    pthread_create(&t, NULL, &directoryHandler, a);
    pthread_join(t, NULL);
	//printTest(bigList);
	
    //Pre-Analysis
    if(bigList->data == NULL)
    {
        printf("No data written\n");
    }
    else if(listLength(bigList)==1)
    {
        printf("Warning: not enough valid entries\n");
    }
    else 
    {
        analyze(bigList);
    }
	
	
	Iterator iter = {bigList};
	while(hasNext(&iter))
	{
		Node* temp = (Node*)(next(&iter)->data);
		deleteList(temp, 3);	
	}
	deleteList(bigList, 0);
	free(bigList);
	free(a);
}