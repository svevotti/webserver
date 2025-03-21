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
If it's the beginning of a server block, start the parseServer (see below). It will return once it reaches the end of the block, add the server to a server vector, and continue reading, unless there was an error, in that case it skips the server block completely (not added to the vector).

------------
|InfoServer|
------------

Stores all the info of the server. At the moment it only has the setter functions


-------
|TO DO|
-------

Currently in the location blocks I don't store anything other than the path, the methods allowed and whether it is internal (i.e. error page), needs to pick up more info if needed (also, should those variables be stored in the locations, or in the main server?)
Need to add more tests to make sure it works with weird stuff (symbols for example)
Need to check for leaks

Check that port and IP are not equal for any server, but either of them can be the same (not simultaneous).
