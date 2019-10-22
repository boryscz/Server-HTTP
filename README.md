# Server HTTP.
C Language Server programming.

## Compilation

1. Open project directory.
2. Write to the terminal:
    * cd /src
    * gcc -pthread server.c -Wall -o server
    * ./server
3. Now your HTTP Server is working.

## Description
This simple HTTP Server handle connection with **client** (Postman or WebBrowser). 

Server URL: http://127.0.0.1:8080

There are implemented methods:
* **PUT /books** (with request body) example:

        {
                "id": 111,
                "author": "New Example",
                "title": "Example",
                "isbn": "99999999",
                "language": "english",
                "numberOfPages": 999,
                "onShelf": true,
                "reader": null
            }
    It will add or modify element in the db.
* **GET /books** it will return full list of content
* **GET /books/id** it will return one element from database (example id=123)
* **DELETE /books/id** it will delete one element from database and put rest of data to the response
* **HEAD /books** it will return header of response 
* **HEAD /books/id** it will return header of response
* **POST /books** (with requst body) example:

        {
                "id": 111,
                "author": "New Example",
                "title": "Example",
                "isbn": "99999999",
                "language": "english",
                "numberOfPages": 999,
                "onShelf": true,
                "reader": null
            }

    It will add element to the database.

# Authors
[Dominik Rolewski](https://github.com/drolewski)

[Mateusz Ratajczak](https://github.com/mateuszratajczak)
