# Unix-Socket-Programming

Simulated a peer-to-peer file-sharing network via ad hoc connections using TCP and UDP sockets. The main phases of the simulation involve patient registration with a server, authentication of content provider, and content sharing.

To run the program, move the following files to the same directory:
  doctor1.txt
  doctor2.txt
  healthcenter.txt
  healthcenter.cpp
  patient.cpp
  doctor.cpp
Change to that directory.
Type in the following commands in terminal:
  g++ -o healthcenter healthcenter.cpp -lsocket -lnsl -lresolv
  g++ -o patient patient.cpp -lsocket -lnsl -lresolv
  g++ -o doctor doctor.cpp -lsocket -lnsl -lresolv
  ./healthcenter &
  ./patient &
  ./doctor &
