#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include "bencode.h"
#include "bt_lib.h"
#include "bt_setup.h"
# define MAX_LENGTH 1024
# define TIMEOUT_SECS 10

struct bt_handshake{
char protocol_id[20];
char reserved[8];
char info_hash[20];
char peer_id[20];
};

char *read_file(const char *file, long long *len)
{
#ifdef FILE_IO
struct stat st;
char *ret = NULL;
FILE *fp;
if (stat(file, &st))
return ret;
*len = st.st_size;
fp = fopen(file, "r");
if (!fp)
return ret;
ret = malloc(*len);
if (!ret)
return NULL;
fread(ret, 1, *len, fp);
fclose(fp);
return ret;
#else
return NULL;
#endif
}
char * initialize_read_bitfieldfile(char * filename,int max_chunck)
{
    FILE *fp;
    char bit_array[max_chunck+1]; 
    char tempbuf[max_chunck+1];
    int n,i;
    if((fp=fopen(filename,"r")) == NULL)
              {
    //If file not exists write a new file and state as all zeros in this file
               //printf("file not exists");
           	 //fclose(fp);
               fp=fopen(filename,"w");
               for (i=0;i<max_chunck;i++) //Initializing array
                    bit_array[i]='N'; //state as 0
            
               fwrite(bit_array,1,max_chunck,fp);
               //memcpy(bit_array,tempbuf,max_chunck);
               fclose(fp);
               
            }
        else
        { 
        //printf("\n #####################################################################################\n");
                             
                            // fp=fopen("test.txt","r");
        int n=fread(tempbuf,1,max_chunck,fp);
        fclose(fp);
        if(n!=max_chunck)
        {
        printf("\n error in number of bit fields stored in file.Some issue with file created for storing state \n ");
        exit(1);
        }
        //If file exists read its contents and store it in this bit_array
        //if(bt_args.verbose) printf(" \n Data in the file is : %s \n",tempbuf);
        memcpy(bit_array,tempbuf,max_chunck);
        }
         bit_array[max_chunck]=NULL;       
         return bit_array;
}

void write_bitfieldstatus(char * filename,int max_chunck,int pid)
{

    FILE *fp;
    int n,i;
    fp=fopen(filename,"r");
    char up_bitarry[max_chunck];
    
    n=fread(up_bitarry,1,max_chunck,fp);
    fclose(fp);
    if (n != max_chunck)
    {
        printf("Error reading from bitfile");
        exit(1);
    }
    else
    {
         fp=fopen(filename,"w");
         up_bitarry[pid-1]='Y';
         n=fwrite(up_bitarry,1,max_chunck,fp);
        if (n != max_chunck)
        {
            printf("Error reading from bitfile");
            exit(1);
        }
                
       fclose(fp); 
    }
    

}
char* compute_hash_for_fileName(char * fileName)
{
/*char data[BUF_SIZE]; //some size
    char digest[SHA_DIGEST_LENGTH];//20 bytes int fd = open(some_file); //some file
    int bytes_read, i;
    bytes_read = read(fd,data,BUF_SIZE);//read data 
    SHA1(data, bytes_read, digest); //compute hash
    */
    char* digest=malloc(sizeof(char)*21);
    int bytes_read, i;
    //bytes_read = read(fd,data,BUF_SIZE);//read data 
   // printf("input for HashTEST %s \n",fileName);
  //  printf("str len of filename %i \n",strlen(fileName));
    digest[20]=NULL;
    SHA1(fileName, strlen(fileName), digest); //compute hash
    
   // printf("Digest for HashTEST %s  strlen(Digest) %i \n",digest,strlen(digest));
    return digest;
}


char* compute_hash_for_buf(char * buf, int length)
{
    char* digest=malloc(sizeof(char)*21);
    int bytes_read, i;
    //bytes_read = read(fd,data,BUF_SIZE);//read data 
   // printf("input for HashTEST %s \n",fileName);
  //  printf("str len of filename %i \n",strlen(fileName));
    digest[20]=NULL;
    SHA1(buf, length, digest); //compute hash
    
   // printf("Digest for HashTEST %s  strlen(Digest) %i \n",digest,strlen(digest));
    return digest;
}
void intial_file_to_write(char* filename, int length)
{
    FILE* fp;
    if((fp=fopen(filename,"r")) != NULL)
        return;
    
    FILE* file=fopen(filename,"wb");
    if (!file){
        perror("Error in create file");
        exit(1);
        }
    
    char* buf= malloc(sizeof(char)* length);
    memset(buf,"0",length);
 
    fwrite(buf,1,(length),file);
    
    //printf("done writing for intial file\n");
    fclose(file);
}

char* read_piece_from_file(char* filename,int pieceid, int piecelength,char* piecehashs,int * bytesreaded)
{ 
     int offset;
    FILE* file=fopen(filename,"rb");
    if (!file){
        perror("Error in Open file");
        exit(1);
        }
    // compute the offest 
     offset= piecelength*(pieceid-1);
     printf("\n pieceid is %d",pieceid);
     printf("\n piece length is %d",piecelength);
     //printf(" \n offset is %d",offset);
    // seek to begining of the piece
    int s=fseek ( file, offset,SEEK_SET );
    if (s!=0){
        perror("Error in reading file");
        exit(1);
    }
    // read the piece
   char* mybuf= malloc(sizeof(char)*piecelength);
    //printf("buffer size is %d",sizeof(mybuf));
    //printf(" \n Inside read_piece_from_file. Now start reading \n");
  
    (*bytesreaded)= fread(mybuf,1,piecelength,file );
    
    //printf("Debug: readed bytes%i", (*bytesreaded) );
    //compute the hash for piece
    char* hashreaded=compute_hash_for_buf(mybuf, (*bytesreaded));
    
    char hashtorr[21];
    hashtorr[20]=NULL;
    memcpy(hashtorr,&piecehashs[(pieceid-1)*20],20);
    //printf(" all torr hash %s \n", piecehashs);
    //printf("hash for piece %s \n", hashtorr);
    //printf("hash for readed %s \n", hashreaded);
    if (strcmp(hashreaded,hashtorr)!=0){
        perror(" Error in hash for readed piece, Not equal the one in torrent\n");
        exit(1);
        }
    
    return mybuf;
    
    
    
}


void write_piece_to_file(char* filename,int pieceid, int piecelength,char* piecehashs, char* buf,int bytestowrite) 
{
    FILE* file=fopen(filename,"r+b");
    if (!file){
        perror("Error in Create file \n");
        exit(1);
        }
    // compute the offest 
    int offset= ( piecelength* (pieceid-1));
    // seek to begining of the piece
    int s=fseek ( file, offset,SEEK_SET );
    if (s!=0){
        perror("Error in seaking in file\n");
        fclose(file);
        exit(1);
    }
    
    // check the hash for the buf and hash in torrent
    //compute the hash for piece
    char* hashreaded=compute_hash_for_buf(buf, (bytestowrite));
    
    char hashtorr[20];
    hashtorr[20]=NULL;
    memcpy(hashtorr,&piecehashs[(pieceid-1)*20],20);
    //printf(" all torr hash %s \n", piecehashs);
    //printf("hash for piece %s \n", hashtorr);
    //printf("hash for write %s \n", hashreaded);
    if (strcmp(hashreaded,hashtorr)!=0){
        perror(" While writing into file Error in hash for readed piece, Not equal the one in torrent\n");
        fclose(file);
        exit(1);
        }
    
    int writtenbytes=fwrite(buf,1,(bytestowrite),file);
    if( writtenbytes != (bytestowrite)){
        perror("Something wrong when write to file, # of written bytes not equal to the number should be written \n");
        exit(1);
        fclose(file);
        }
    
    fclose(file);
}
    


    

void check_parse(char* fileName)
{
   //http://sourceforge.net/p/funzix/code/ci/master/tree/bencode/test.c#l43
    

char *buf;
long long len;
be_node *n;
buf = read_file(fileName, &len);
if (!buf) {
buf = fileName;
len = strlen(fileName);
}
printf("DECODING: %s\n", fileName);
n = be_decoden(buf, len);
if (n) {
be_dump(n);
be_free(n);
} else
printf("\tparsing failed!\n");
#ifdef FILE_IO
if (buf != fileName)
free(buf);
#endif 
return;
    
}

void parseNode(be_node *node, size_t indent ,bt_info_t* my_bt_info )
{
    
        char* filename;
        int piecelength; 
        char* hashpieces;
        int hashlen; 
        int filelength;
	 size_t i;
        
	_be_dump_indent(indent);
	indent = abs(indent);
        
        switch (node->type) {
		case BE_STR:
			//printf("AAstr = %s (len = %lli)\n", node->val.s, be_str_len(node));
                        //if(strcmp(node->val.s,"announce")==0)
                        //        strcpy(my_bt_info->announce,node->val.s);

			break;

		case BE_INT:
			//printf("int = %lli\n", node->val.i);
			break;

		case BE_LIST:
			//puts("list [");

			for (i = 0; node->val.l[i]; ++i)
				_be_dump(node->val.l[i], indent + 1);

			_be_dump_indent(indent);
			//puts("]");
			break;

		case BE_DICT:
			//puts("dict {");

			for (i = 0; node->val.d[i].val; ++i) {
				_be_dump_indent(indent + 1);
				//printf("%s=> ", node->val.d[i].key);
                                
                                if(strcmp(node->val.d[i].key,"announce")==0)
                                strcpy(my_bt_info->announce,node->val.d[i].val->val.s);
                                
                                if (strcmp(node->val.d[i].key , "name")==0)
                                {
                                    //node->val.d[i].val->val
                                    //printf("MMMMM ;;;;; name %s\n",node->val.d[i].val->val.s);
                                    filename=node->val.d[i].val->val.s;
                                    strcpy(my_bt_info->name,filename);

                                    //Hash of the file name 

                                    //my_bt_info->name= filename;
                                    //printf("MMMMM ;;;;; name %s\n",filename);
                                }
                                
                                if (strcmp(node->val.d[i].key , "piece length")==0)
                                {
                                    //node->val.d[i].val->val
                                    //printf("MMMMMMM;;;;; piece length %i\n",node->val.d[i].val->val.i);
                                    piecelength=node->val.d[i].val->val.i;
                                    my_bt_info->piece_length=piecelength;
                                    
                                    //printf("MMMMMMM;;;;; piece length %i\n",piecelength);
                                }

                                if (strcmp(node->val.d[i].key , "pieces")==0)
                                {
                                    //node->val.d[i].val->val
                                    //printf("hash pieces %s\n",node->val.d[i].val->val.s);
                                    hashpieces=node->val.d[i].val->val.s;
                                    hashlen= be_str_len(node->val.d[i].val);
                                    my_bt_info->num_pieces= (hashlen/20);
                                    //my_bt_info->piece_hashes = malloc( my_bt_info->num_pieces * sizeof(char*));
                                    my_bt_info->my_peiecehases= hashpieces;
                                    //strcpy(my_bt_info->my_peiecehases,hashpieces);
                                   // printf("MMMMM;;;;;; hash pieces %s -- hashlen %i\n",hashpieces,hashlen);
                                }
                                
                                if (strcmp(node->val.d[i].key , "length")==0)
                                {
                                    filelength=node->val.d[i].val->val.i;
                                    my_bt_info->length=filelength;
                                    //printf("MMMMM;;;;;; filelen %i\n",filelength);
                                }
                                
				parseNode(node->val.d[i].val, -(indent + 1),my_bt_info);
			}

			_be_dump_indent(indent);
			//puts("}");
			break;
	}
}




void client(bt_args_t *bt_args,bt_info_t* my_bt_info,char* bitfilename)
{
		struct bt_handshake handshake;
		int i,n;
		char handshake_str[68];
		char leecher[20];
		char seeder[20];
		char *seederfromhandshake;
		char tempbuf[20];
		int sockfd;
		char receivedbuf_handshake[68];
                FILE *fp1; 
                char* readbuf;
			//creating scoket 
                int max_chunck=bt_args->bt_info->num_pieces;
		 if(bt_args->logfile){
                //if(fopen(bt_args->log_file,"r")!=NULL) fp1=fopen(bt_args->log_file,"w");
                //else  fp1=fopen(bt_args->log_file,"a");
                    fp1=fopen(bt_args->log_file,"a");
                    }
                //char* bitfilename="Restart.tmp";
                char* bit_down = malloc(sizeof(char)*max_chunck);
                bit_down=initialize_read_bitfieldfile(bitfilename,max_chunck);
                    
            //if(bt_args->logfile) fprintf(                      
		if((sockfd=socket(AF_INET,SOCK_STREAM,0))== -1)
			{
					perror("socket");
					exit(1);
			}
			// print_peer(bt_args->peers[1]);

			//printf(" \n  sockaddr size is %d",sizeof(struct sockaddr_in));

			//printf(" \n peer size is %d",sizeof(bt_args->peers[1]->sockaddr));

			//printf("\n IP is %s",inet_ntoa(bt_args->peers[1]->sockaddr.sin_addr));

			//printf("\n Port is %d",ntohs(bt_args->peers[1]->sockaddr.sin_port));

			//Initialize the structure
		if(connect(sockfd,(struct sockaddr *) &(bt_args->peers[1]->sockaddr),sizeof(struct sockaddr_in)) < 0)
		{
				perror("connect");
				exit(1);

		}

		//Preparing handshake message

            strcpy(handshake.protocol_id,"19BitTorrentProtocol");

            strcpy(handshake.reserved,"00000000");



	   if(bt_args->logfile) fprintf(fp1,"fileName to compute the hash : %s \n",bt_args->bt_info->name);
            char * fileHash = compute_hash_for_fileName(bt_args->bt_info->name);


	 if(bt_args->logfile) fprintf(fp1,"Handshake fileHash : %s  , len of filehash %i\n",fileHash,strlen(fileHash));

            strcpy(handshake.info_hash,fileHash);


	//printf(" Host : %s \n ",bt_args->peers[1]->id); 

	//printf(" Host client  Port : %d \n ",bt_args->peers[1]->port);
	
	calc_id("127.0.0.1",bt_args->peers[1]->port,leecher);


	strcpy(handshake.peer_id,leecher);

      if(bt_args->logfile)  fprintf(fp1," \n peer id of leeecher is %s",leecher);
	
       memset(&leecher,0,sizeof(leecher));
       memset(&handshake_str,0,sizeof(handshake_str));
    
	memcpy(handshake_str,&handshake,68);

	//printf(" buffer copied from structure is : %s \n",handshake_str);
	//printf("size of handshake_str is %d",strlen(handshake_str));

		//send handshake
  			  
	n=write(sockfd,handshake_str,68);

        if(n<=0){
                perror("write failed");
                exit(1);
        }
       if(bt_args->logfile){
            fprintf(fp1,"\n handshake request sent to seeder");
            fprintf(fp1,"\n %d bytes sent to server",n);
        }
	//clear the sent buffer 
        memset(&handshake_str,0,sizeof(handshake_str));
        memset(&handshake,0,sizeof(handshake));
	 //receive handshake
	while(1)
    	{
 	 	 n=read(sockfd,receivedbuf_handshake,68);
 		 if(n<0)
		  {
	  		perror("read failed");
		 	 exit(1);
	 	  }
         
		if(bt_args->logfile){

                    fprintf(fp1," \n \n Handshake response is %s \n",receivedbuf_handshake);
                    fprintf(fp1," \n seeder port taken is %d \n ",bt_args->peers[1]->port);
                }
              //calculating peer id of leecher
              char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 stri
              inet_ntop(AF_INET,&(bt_args->peers[1]->sockaddr.sin_addr),ip4, INET_ADDRSTRLEN);
              // printf("The IPv4 address is: %s\n", ip4);
		calc_id(ip4,bt_args->peers[1]->port,seeder); //need to update this with seeder ip

		//Fetch the peer id from buffer
	
		memset(&tempbuf,0,sizeof(tempbuf));
              	if(bt_args->logfile){
                 fprintf(fp1," Seeder Peer ID %s \n",seeder);
                 fprintf(fp1,"\n sockfd at peer id verification handshake is %d",sockfd);
                      }
		memcpy(tempbuf,receivedbuf_handshake+48,20);
                tempbuf[20]=NULL;
  
            
              if(strcmp(tempbuf,seeder)>0){
		printf(" peer_id of seeder not expected.So dont want to continue to download\n");
              if(bt_args->logfile) fprintf(fp1," peer_id of seeder not expected.So dont want to continue to download\n");
		exit(1);
		}
              //Clear all the buffers and char arrays used.
  		memset(&receivedbuf_handshake,0,sizeof(receivedbuf_handshake));
              memset(&tempbuf,0,sizeof(tempbuf));

            if(bt_args->logfile)             fprintf(fp1,"Handshake Complete");
      
        
               /* verbose mode if() 
  	printf("\nReceived handshake from: %s\n", bt_args.peers[i]->ip);*/
 

        int piece_length=bt_args->bt_info->piece_length;
        long   plength=bt_args->bt_info->piece_length;
        char* phashs=bt_args->bt_info->my_peiecehases;
  
   
     //random_pick(sockfd,plength,phashs,max_chunck);
        char * filenamew="TempTemp";
        if (bt_args->save_file != NULL)
            filenamew= bt_args->save_file;

        int piece_id;
         long sizefiletillnow=0; 
           
           
          printf("\n Handshake IS DONE  and start to download file\n");
         for (piece_id=1;piece_id <=max_chunck; piece_id=piece_id+1)
        // for (piece_id = max_chunck ; piece_id >= 1; piece_id=piece_id-1)
         
         {
            
            
            char* nbitdown=initialize_read_bitfieldfile(bitfilename,max_chunck);
             
            if (nbitdown[piece_id-1]=='Y')
                   {   
              printf("this piece already downloaded before\n"); 
              if(bt_args->logfile){
              fprintf(fp1,"\n This piece already downloaded before\n"); 
              fprintf(fp1,"\n str  nbitdown %s  ---\n",nbitdown);
                }
                       sizefiletillnow=sizefiletillnow+plength;
                       continue;
                       }
            int data = htonl(piece_id);
            char tbuf[plength];                                                
            if(write(sockfd,&data,sizeof(data))>0)
                    {
                    
                    
                        
                        if (plength > bt_args->bt_info->length)
                        {
                            plength=bt_args->bt_info->length;
                             
                        }
                        char myrbuf[plength];
                        int s=0;
                      //  printf(" BEfore now we reading from server on sockfd %i \n",sockfd);
                        while (s<plength && sizefiletillnow< bt_args->bt_info->length )
                        {
                            n=read(sockfd,tbuf,plength-s);
                            sizefiletillnow=sizefiletillnow+n;
                            if (s<plength)
                            {
                                memcpy(&myrbuf[s],tbuf,n);
                            }
                            s=s+n;
                               
                          // printf(" NUMBER OF BYTES READED  %i \n",n); 
                            //printf(" Size of file   %i and dowloaded till now %i \n",bt_args->bt_info->length,sizefiletillnow); 
                            /* Client not able to get out of this loop 
                        }
                        
                        
                       //  printf(" Total NUMBER OF BYTES READED  %i \n",s); 
                            
                        
                      // printf(" now we reading from server on sockfd %i \n",sockfd);
                        //printf(" NUMBER OF BYTES READED  %i \n",n); 
                       // printf("Recieved Buffer is  %s",myrbuf); 
                         if(n<0)
                          {
                        perror("read failed");
                         exit(1);
                         }
                        if(bt_args->logfile){
                       fprintf(fp1,"\n\n\n Wiritng to given file for piece_id %i \n",piece_id);
                       fprintf(fp1,"\n\n\n Number of bytes in this  piece_id %i \n",s);
                       fprintf(fp1,"\n\n\n Piece length for this   piece_id %i \n",plength);
                            }
                        
          
                        write_piece_to_file(filenamew,piece_id, plength,phashs,myrbuf,s);         
                        write_bitfieldstatus(bitfilename,max_chunck,piece_id);
                        double ans = (double)(piece_id)/ (double)(max_chunck);
                        printf("\n Progress of download is %f \n", ans );
                    }
                }
            
      // doing it for one piece;
        
    
      //  n= write(sockfd,"10",2);
          if(bt_args->logfile)  {fprintf(fp1,"\n downloaded the file \n \n"); fclose(fp1);}
            exit(0);
              
         } //end of while 

}

/*int find_piece_chunk_to_download(int max_chunck,bt_args_t *bt_args)
{
              char bit_array[max_chunck];
              int i,chucnk_selected;
              chucnk_selected=rand() % max_chunck +1;

}*/


void run_seeder_for_client(bt_args_t *bt_args,bt_info_t* my_bt_info,int newsockfd)
{
    
    char handshakebuffer[68];
    struct sockaddr_in servAddr, cli_addr;
    int  n;
    char buffer[MAX_LENGTH];
    char seederid[20];
    struct bt_handshake seeder_handshake;
    char bytes[100];
    char tempbuf[100];
    char leecher_handshake[68];
    //int * bytesreaded= malloc(sizeof(int));
    int bytesreaded=0; 
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(INIT_PORT);  //some issue here 
    
    //char* tbuff= malloc(bt_args->bt_info->piece_length);
    //char* tbuff= malloc(sizeof(char)* bt_args->bt_info->piece_length);   
    //memset(tbuff, 1, bt_args->bt_info->piece_length);
    char tbuff[bt_args->bt_info->piece_length];   

    //Prepare handshake structure of seeder

    //printf("\n %d size of seeder \n ",sizeof(seeder_handshake));
    strcpy(seeder_handshake.protocol_id,"19BitTorrentProtocol");
    strcpy(seeder_handshake.reserved,"00000000");
    char* fileHash = compute_hash_for_fileName(bt_args->bt_info->name);
    strcpy(seeder_handshake.info_hash,fileHash);


    calc_id("127.0.0.1",INIT_PORT,seederid); // issue here (peer_id is not matched)  htons is removed 
    strcpy(seeder_handshake.peer_id,seederid);

    //printf(" \n peer id of seeder is %s \n",seederid);
    //copy into buffer
    memset(&handshakebuffer, 0, sizeof(servAddr));
    memcpy(handshakebuffer,&seeder_handshake,68);
    //printf(" \n seeder peer id is %s",seederid);
    //printf("\n %d  bytes from structure ",strlen(handshakebuffer));
    //printf("\n %d size of seeder \n ",sizeof(seeder_handshake));
    //printf("\n  seeder Port after intializing is %d",ntohs(servAddr.sin_port));

    
    
    
    ///////
    
    while(1)
    {
        //Accept incoming connections
        

        n = read(newsockfd,leecher_handshake,68); //Read handshake message
        if (n < 0)
        {
                perror("ERROR reading from socket");
                close(newsockfd);
                //exit(1);
                return;
       	 }
        if(bt_args->verbose){
        printf("\n %d  bytes received at seeder end \n",n);
        printf("\n sent from leecher :%s ",leecher_handshake);       
        printf(" \n \n buffer copied from structure is : %s \n",handshakebuffer);
        printf("\n %d size of seeder \n ",sizeof(seeder_handshake));
        }
        // Fill in the handshake structure and compare
       memcpy(tempbuf, leecher_handshake, 20);
       printf(" temp buf:%s \n",tempbuf);
       
        if(strcmp(tempbuf,seeder_handshake.protocol_id)>0){
            printf("Not a bittorrent protocol \n\n\n");
            write (newsockfd,"Not a bittorrent protocol",100);
            close(newsockfd);
				//exit(1);
        }
        else{
        printf(" \n bit torrent handshake header ok");
        }


        memset(&tempbuf,0,sizeof(tempbuf));
        memcpy(tempbuf, leecher_handshake+20,8);
      if(bt_args->verbose) printf(" temp buf:%s \n",tempbuf);
      if(bt_args->verbose) printf("\n sent from leecher :%s  \n",seeder_handshake.reserved);
      if(strcmp(tempbuf,seeder_handshake.reserved)>0){

            if(bt_args->verbose) printf("Info hash value not matched.Server dont have the file");
            write (newsockfd,"",100);
            close(newsockfd);
            //exit(1);
            return;
            }
        else{
		if(bt_args->verbose) printf(" \n bit torrent Resserve field  ok");
            }

        memset(&tempbuf,0,sizeof(tempbuf));
        memcpy(tempbuf, leecher_handshake+28, 20);
        
        if(strcmp(tempbuf,seeder_handshake.info_hash)>0){

            if(bt_args->verbose) printf("Info hash value not matched.Server dont have the file");
            write (newsockfd,"Info hash value not matched.Seedert have the file",100);
            close(newsockfd);
            //exit(1);
	}
        else{
            if(bt_args->verbose) printf(" \n bit torrent Info Hash ok");
        }
        memset(&tempbuf,0,sizeof(tempbuf));
        memset(&leecher_handshake,0,sizeof(leecher_handshake));


    //end the handshake message
     n = write(newsockfd,handshakebuffer,68);
     if (n < 0){
        perror("ERROR writing to socket ");
        close(newsockfd);
        return;
                
        
    }   
    if(bt_args->verbose) printf("\n %d bytes sent to client",n);
    //Clear the handshake buffer
    memset(&handshakebuffer,0,sizeof(tempbuf));
    
      printf("  \n Handshake for client done correctly \n ");
     while(1)
        {
          int piece_id=0;
          
           // printf("\n no data received upto now");
           // wait for leecher to respond 
            n = read(newsockfd,&piece_id,sizeof(piece_id)); //Read piece id  message
            if (n <= 0 ){
                perror("No reading from socket && close socket and wait for new connection");
                //exit(1);
                close(newsockfd);
                return;
                }       
           
           if (n>0)
           {
             
           // printf("\n Data sent by receiver after handsake is %d",piece);
                 char* buf2=read_piece_from_file(bt_args->bt_info->name,ntohl(piece_id),bt_args->bt_info->piece_length,bt_args->bt_info->my_peiecehases,&bytesreaded);
          
      
                
                 int s=0;
                 while (s<bytesreaded)
                 {
                 n=write(newsockfd,buf2,bytesreaded);
                 s=s+n;
                 printf("\n %d bytes sent from seeder for piece %d \n",n,ntohl(piece_id));
                 sleep(3);
                 }
		}   
            } // end  of while

 } //end of while 

    
    //////

}


void seeder(bt_args_t *bt_args,bt_info_t* my_bt_info)
{
   
    int pid;
    char handshakebuffer[68];
    struct sockaddr_in servAddr, cli_addr;
    int  n;
    char buffer[MAX_LENGTH];
    char seederid[20];
    struct bt_handshake seeder_handshake;
    char bytes[100];
    char tempbuf[100];
    char leecher_handshake[68];
    //int * bytesreaded= malloc(sizeof(int));
    int bytesreaded=0; 
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(INIT_PORT);  //some issue here 
    
    //char* tbuff= malloc(bt_args->bt_info->piece_length);
    //char* tbuff= malloc(sizeof(char)* bt_args->bt_info->piece_length);   
    //memset(tbuff, 1, bt_args->bt_info->piece_length);
    char tbuff[bt_args->bt_info->piece_length];   

    //Prepare handshake structure of seeder

    if(bt_args->verbose) printf("\n %d size of seeder \n ",sizeof(seeder_handshake));
    strcpy(seeder_handshake.protocol_id,"19BitTorrentProtocol");
    strcpy(seeder_handshake.reserved,"00000000");
    char* fileHash = compute_hash_for_fileName(bt_args->bt_info->name);
    strcpy(seeder_handshake.info_hash,fileHash);


    calc_id("127.0.0.1",INIT_PORT,seederid); // issue here (peer_id is not matched)  htons is removed 
    strcpy(seeder_handshake.peer_id,seederid);

    if(bt_args->verbose) printf(" \n peer id of seeder is %s \n",seederid);
    //copy into buffer
    memset(&handshakebuffer, 0, sizeof(servAddr));
    memcpy(handshakebuffer,&seeder_handshake,68);
    //printf(" \n seeder peer id is %s",seederid);
    //printf("\n %d  bytes from structure ",strlen(handshakebuffer));
    if(bt_args->verbose) printf("\n %d size of seeder \n ",sizeof(seeder_handshake));
    //printf("\n  seeder Port after intializing is %d",ntohs(servAddr.sin_port));

    int seederfd=socket(AF_INET,SOCK_STREAM,0);
    if(seederfd==-1)
    {
    perror("socket");
    exit(1);
    }

    // printf(" \n socket created and its value is %d \n",seederfd);
    // printf("\n  seeder Port is %d",ntohs(servAddr.sin_port));

    // printf(" \n  sockaddr size is %d",sizeof(struct sockaddr_in));

    // printf(" \n seeder peer size is %d",sizeof(servAddr));

    // printf("\n  seeder IP is %s",inet_ntoa(servAddr.sin_addr));

    // printf(" \n socket created");

    if (bind(seederfd, (struct sockaddr*) &servAddr, sizeof(servAddr))>0)
    {    
    perror("Bind");
    exit(1);
    }
    listen(seederfd,MAX_CONNECTIONS);

    while(1)
    {
        
        
        //Accept incoming connections
         socklen_t clntAddrLen = sizeof(cli_addr);
         int newsockfd = accept(seederfd,(struct sockaddr *) &cli_addr, &clntAddrLen);
         if (newsockfd < 0){
                  perror("error on accept");
                  close(seederfd);
                   exit(1);
            }

        //else
        //run_seeder_for_client(bt_args,my_bt_info,newsockfd);
        
        
        
        /* Create child process */
        pid = fork();
        if (pid < 0)
        {
            perror("error on fork");
            close(seederfd);
	    exit(1);
        }
        if (pid == 0)  
        {
            close(seederfd);
            
            run_seeder_for_client(bt_args,my_bt_info,newsockfd);
            
            exit(0);
        }
        else
        {
            close(newsockfd);
        }
        
        
        
        
        
        
        /*
        //printf("\n \n IP is %s",inet_ntoa(cli_addr.sin_addr));

        //printf("\n Port is %d",ntohs(cli_addr.sin_port));

        //printf("\n IP is %s",inet_ntoa(cli_addr.sin_addr));


        n = read(newsockfd,leecher_handshake,68); //Read handshake message
        if (n < 0)
        {
                perror("ERROR reading from socket");
                exit(1);
       	 }

        printf("\n %d  bytes received at seeder end \n",n);

        printf("\n sent from leecher :%s ",leecher_handshake);
        
        printf(" \n \n buffer copied from structure is : %s \n",handshakebuffer);

        printf("\n %d size of seeder \n ",sizeof(seeder_handshake));

        // Fill in the handshake structure and compare
       memcpy(tempbuf, leecher_handshake, 20);
       printf(" temp buf:%s \n",tempbuf);
       
        if(strcmp(tempbuf,seeder_handshake.protocol_id)>0){
            printf("Not a bittorrent protocol \n\n\n");
            write (newsockfd,"Not a bittorrent protocol",100);
            close(newsockfd);
				//exit(1);
        }
        else{
        printf(" \n bit torrent handshake header ok");
        }


        memset(&tempbuf,0,sizeof(tempbuf));
        memcpy(tempbuf, leecher_handshake+20,8);
      printf(" temp buf:%s \n",tempbuf);
      printf("\n sent from leecher :%s  \n",seeder_handshake.reserved);
      if(strcmp(tempbuf,seeder_handshake.reserved)>0){

            printf("Info hash value not matched.Server dont have the file");
            write (newsockfd,"",100);
            close(newsockfd);
            //exit(1);
            }
        else{
		printf(" \n bit torrent Resserve field  ok");
            }

        memset(&tempbuf,0,sizeof(tempbuf));
        memcpy(tempbuf, leecher_handshake+28, 20);
        
        if(strcmp(tempbuf,seeder_handshake.info_hash)>0){

            printf("Info hash value not matched.Server dont have the file");
            write (newsockfd,"Info hash value not matched.Seedert have the file",100);
            close(newsockfd);
            //exit(1);
	}
        else{
            printf(" \n bit torrent Info Hash ok");
        }
        memset(&tempbuf,0,sizeof(tempbuf));
        memset(&leecher_handshake,0,sizeof(leecher_handshake));


    //end the handshake message
     n = write(newsockfd,handshakebuffer,68);
     if (n < 0){
        perror("ERROR writing to socket");
        exit(1);
    }   
    printf("\n %d bytes sent to client",n);
    //Clear the handshake buffer
    memset(&handshakebuffer,0,sizeof(tempbuf));
    
    //  printf(" \n \n Reached end of seeder handshake ");
    while(1)
        {
          int piece_id=0;
          
           // printf("\n no data received upto now");
           // wait for leecher to respond 
            n = read(newsockfd,&piece_id,sizeof(piece_id)); //Read piece id  message
            if (n <= 0 ){
                perror("No reading from socket && close socket and wait for new connection");
                //exit(1);
                close(newsockfd);
                break;
                }       
           
           if (n>0)
           {
             
           // printf("\n Data sent by receiver after handsake is %d",piece);
                 char* buf2=read_piece_from_file(bt_args->bt_info->name,ntohl(piece_id),bt_args->bt_info->piece_length,bt_args->bt_info->my_peiecehases,&bytesreaded);
          
      
                
                 int s=0;
                 while (s<bytesreaded)
                 {
                 n=write(newsockfd,buf2,bytesreaded);
                 s=s+n;
                 printf("\n %d bytes sent from seeder for piece %d \n",n,ntohl(piece_id));
                 //sleep(3);
                 }
		}   
            } // end  of while
            
            
            */
            
            

 } //end of while 
		 
}

int main (int argc, char * argv[]){

  bt_args_t bt_args;
  be_node * node; // top node in the bencoding
  int i;
  bt_info_t* my_bt_info = malloc(sizeof(my_bt_info));  
  parse_args(&bt_args, argc, argv);

    

// To compare with server response we need this bt_args.peers[1]->id

//struct bt_handshake handshake;
//char *str_handshake;


  if(bt_args.verbose){
    printf("Args:\n");
    printf("verbose: %d\n",bt_args.verbose);
    printf("save_file: %s\n",bt_args.save_file);
    printf("log_file: %s\n",bt_args.log_file);
    printf("torrent_file: %s\n", bt_args.torrent_file);

    for(i=0;i<MAX_CONNECTIONS;i++){
      if(bt_args.peers[i] != NULL)
        print_peer(bt_args.peers[i]);
    }
    
  }

  //read and parse the torent file
  node = load_be_node(bt_args.torrent_file);
  
  //check_parse(bt_args.torrent_file);
  
  /*
  parseNode(node,0,my_bt_info);
  
  
  ///// Print the datat from the sturcure
  printf("my_bt_info->announce %s\n",my_bt_info->announce);
  printf("my_bt_info->name  %s\n",my_bt_info->name);
  printf("my_bt_info->piece_length  %i\n",my_bt_info->piece_length);
  printf("my_bt_info->length  %i\n",my_bt_info->length);
  printf("my_bt_info->num_pieces  %i\n",my_bt_info->num_pieces);
*/
  
  
  bt_args.bt_info=malloc(sizeof(bt_info_t));
  parseNode(node,0,bt_args.bt_info);
  if(bt_args.verbose){
      
  printf("my_bt_info->announce %s\n",bt_args.bt_info->announce);
  printf("my_bt_info->name  %s\n",bt_args.bt_info->name);
  printf("my_bt_info->piece_length  %i\n",bt_args.bt_info->piece_length);
  printf("my_bt_info->length  %i\n",bt_args.bt_info->length);
  printf("my_bt_info->num_pieces  %i\n",bt_args.bt_info->num_pieces);
  //printf("log file option \n");  
  printf("log file option %i \n",bt_args.logfile);
  }
 // printf("my_bt_info->my_peiecehases  %s\n",my_bt_info->my_peiecehases);

  //printf(" \n size : %d",sizeof(my_bt_info->my_peiecehases));
  //be_dump(node);
  

  if(bt_args.verbose){
    be_dump(node);
  }

printf("\n Writing detailed information to log file  %s \n",bt_args.log_file);
 if(bt_args.client)
{
char * filenamew="TempTemp1.txt";
char* bitfilename= malloc(1000);
//bitfilename=NULL;
if (bt_args.save_file != NULL)
  {
      filenamew= bt_args.save_file;
      strcpy(bitfilename,filenamew);
    }
strcat(bitfilename,"Restart.tmp");

//printf("fielname for restart %s",bitfilename);
intial_file_to_write(filenamew, bt_args.bt_info->length);
client(&bt_args,my_bt_info,bitfilename);
}
if(bt_args.client==0)
{
    
printf("\n Running as server \n");    
seeder(&bt_args,&bt_args.bt_info);

}
/*
  //main client loop
  printf("Starting Main Loop\n");
  while(1){

    //try to accept incoming connection from new peer
       
    
    //poll current peers for incoming traffic
    //   write pieces to files
    //   udpdate peers choke or unchoke status
    //   responses to have/havenots/interested etc.
    
    //for peers that are not choked
    //   request pieaces from outcoming traffic

    //check livelenss of peers and replace dead (or useless) peers
    //with new potentially useful peers
    
    //update peers, 

  }*/

  return 0;
}
