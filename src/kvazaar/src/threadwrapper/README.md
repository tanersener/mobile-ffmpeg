ThreadWrapper
=======
Wraps pthread functions so that they actually call C++ standard functions.

Only functions used by Kvazaar, an open-source HEVC encoder, are implemented. 
People are free to contribute if they implement other functions.
