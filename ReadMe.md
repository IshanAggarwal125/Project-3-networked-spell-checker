# Project-3-F23  
## Project 3 Networked spell checker
 
1. Execution instructions:

Input make in the command line and it will create a an executable file for the client and 
the server. 

For the server you will need three parameters. `
    1) ./server 
    2) dictionary.txt, or some other .txt file you want to test
    3) portNumber, make sure this port number is the same for the server.
    4) number of worker threads.
    5) number of cells in conenction queue.
    6) The scheduling type FIFO/priority.

2. Design

The code intially starts with getting the paramters from the cmdline.
Next, the clients join and they are placed in a connection queue
    This is where other thing are also happening like 
    1. worker threads, and log threads are being created. 
    2. intializeConnectionQueue
    3. initializeLogQueue
    4. laoding dictionary

Next we go to the worker thread function where based on scheduling we do either FIFO or priority.

The worker thread schedules the correct client, and then collects response from the client, 
until the client exit's. Furthermore, we check the response from the client and then add it to the
log queue.

The log queue function gets the response from the log queue and adds it to the file.

please read the description on top of each function for more details

3. Testing

After writing the smallest piece of code I would test my code. There can be many errors which can come across threads like memory leakage, connection error, binding erorr, deadlocks, race condition etc. I was repeatedly testing my code in the server. 


