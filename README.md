# webserver

------
|main|
------

Initiates the server using the deault.conf file
Checks that the info is accessible using a testing function (can be deleted)


--------
|Config|
--------

The config class is constructed using the config path, and initiates the parsing.
The main structure for parsing is:
Read a line, clean it, ignore empty or comment lines
If it's the beginning of a server block, start the parseServer (see below). It will return once it reaches the end of the block, add the server to a server vector, and continue reading, unless there was an error, in that case it skips the server block completely (not added to the vector)

------------
|InfoServer|
------------




-------
|TO DO|
-------
