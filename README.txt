EPL371: Askisi 4

USER GUIDE:

	1. Compiling:
	
			Users have to compile the source code before use i.
			There is a make file in source code, user has to open
			a terminal type make to compile the system.
	
	2. Configuration File:
			
			Server reads a configuration file to set the number
			of threads for service and the which port is going to 
			listen for requests.
			You can modify the configuration file but dont change the
			format of the file.
	
	4. Resources:
			
			All resources that server gives is directory WWW
			and anything you need to server to clients you have 
			to save it in the WWW directory.
	
	5. Start:
	
			To run the system you have to open a terminal
			(in same directory with executable) and type
			./myserver 
	
	6. Test:
	
			When the server is running you can test it with telnet!
			To connect to the server using telnet give as arguments
			localhost and <port> (same configuration file )
			and make request to server.
	
	7. Implemented Request:
	
			Server can only services GET , DELETE , HEAD request
			so in the telnet you can use this method.
			Other wise server response with not implement method 
			to inform the user.
			if in get request dont give the specific file server
			response with it works.
			 
	8. Conclusion:
