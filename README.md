## Peer-to-peer application
Learning to connect to a server and communicate between two hosts.

### Description
Peer2.cpp is the inital design. I ran into an issue with how the sendRequest() was handling the response from the server. It is a binary being passed back so it needed to be converted into\
a readable form for the user. Also needed to take into account if we are only receiving partial data.\
Peer.cpp is the final fully functional version.\ 
Makefile is provided as well.\
