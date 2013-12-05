Author: Ryan Kophs
Date: 4 Dec 2013
Email: ryan.kophs@colorado.edu

Description:
Peer to peer server. A client may issue 3 commands:
	$ ls					#Get file listing of all peer clients from server
	$ get <filename> <peer IP> <peer Port>	#Download specified file from specified client
	$ exit					#Exit the client
Everytime a get command is sucessfully complete, a new file ledger for that client reflecting the new changes is pushed to the server. The server then pushes the new master file ledger to every listening peer. 

Notes:
1. If get is executed on a non-existent file, nothing will occur. 
2. If the client executes a get command on a file that matches a file already existent in its directory, nothing will occur.
3. Two clients may not use the same port or name on initiation.

Compile:
	$ make clean	#Reset all client file dirs and remove executables
	$ make		#Recompile executables and copy over files into client dirs
	
Run:
Open several terminals, 1 for the server and one for each client. Note the server must start before any of the clients.
	Server Terminal:
	$ cd server			#Move into server directory
	$ ./server_PFS <Server Port>	

	Each Client Terminal:
	$ cd <Client dir>		#Move into appropriate file dir (e.g. A or B)
	$ ./../client/client_PFS <Client Name> <Client Port> <Server IP> <Server Port>


