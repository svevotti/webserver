# webserver

Little explanation of this part (branch sveva/server):
Creation of server that parses HTTP client request, and responses OK to any client who connects and uses GET method

1. SocketServer: creation of server socket and all clients sockets. Handling multiple clients, able to receive and send data

2. ServerParseRequest: parsing HTTP request from client

3. ServerResponse: validating request to any incoming client with GET request to localhost:8080 - html just syntax for header.

How it works:

After make and creating the binary, just execute ./server, and the server will start.
You can either type these commands in terminal

a. curl localhost:8080 (on my mac I have to run this command with these flags: --ipv4 --http0.9) - it is simple GET request with curl command.
You see on the screen the server response and message body (html)
if you are interested in curl command - https://curl.se/docs/tutorial.html, https://www.geeksforgeeks.org/curl-command-in-linux-with-examples/

b. telnet localhost 8080 - same as curl, create connection, but then starts its own "interface" - can just type GET and it shows HTTP response.
to exit telnet the command is quit.

c. localhost:8080 on any web browser and you see the string on the screen.

To close server, for now just ctrl C. Not hadling signals yet!

What I think we are missing
III.3 Configuration file
I just read through this section, nothing has been done.
Also, the parsing of the configuration file is missing


For now, I just shared the links that I found most useful to create a server, just to get an idea of what the project should
look like:
Tutorials
	Server from scratch
	https://bhch.github.io/posts/2017/11/writing-an-http-server-from-scratch/
	https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa
	https://medium.com/@aryandev512/i-wrote-a-http-server-from-scratch-in-c-0a97e8252371
	Socket programming
	https://www.bogotobogo.com/cplusplus/sockets_server_client.php
	Network Programming
	https://beej.us/guide/bgnet/html/#client-server-background
	http://do1.dr-chuck.net/net-intro/EN_us/net-intro.pdf
	https://www.cisco.com/c/en/us/solutions/enterprise-networks/what-is-network-programming.html
