# CNT4007 Project 2

Authors:

- Matthew Alangadan
- Kaniel Vicencio

## Setup the Project

Run the `build.sh` script to build the project. You might need to enable execute permissions for this script.

    $ ./build.sh

## Running the Project

Run the server:

    cd serverFolder
    ./server

Run the first client in a different terminal:

    cd clientFolder
    ./client

Run the second client on another terminal:

    cd multipleConnection
    ./client

Note: make sure to run the client and server in their respective directories (cd into them) or they will not work.

## Folder Structure

There two client folders to test multiple connections.

The `clientFolder` contains a song with the same name but different content and the `multipleConnection` folder contains a song that has a different name but same content.

Both the client and the server contain directories to store files located in `ClientStore` and `ServerStore` respectivley.

## Running more than two clients

If you want to test with more clients, the client folder needs to have its own `ClientStore` directory.
