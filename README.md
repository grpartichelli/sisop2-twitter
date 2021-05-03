Sisop2 Twitter

How to use:

//COMPILE
make clean
make

//RUN SERVER
./server <path_to_config_file> <id_of_server>

//RUN CLIENT
./client <profile> <server_address> <primary_port>



//The config file works as follow:
//Each line represents an RM, the first number is its ID, the second number its port, and the third indicates if its primary (1) or backup (0)

//EXAMPLE:

0 4000 1
1 4001 0
2 4002 0

//Represents 3 RMs, one primary of ID 0 and port 4000, and two backups of IDs 1 2 and ports 4001 4002