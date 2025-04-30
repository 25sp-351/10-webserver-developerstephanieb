# Web Server

---

## Module Structure
|         File          | Purpose                                                                |
| :-------------------: | :--------------------------------------------------------------------- |
|      webserver.c      | Main entry point: sets up the server, accepts clients, creates threads |
|    request.h & .c     | Parses the HTTP request line and headers into a structured format      |
|    response.h & .c    | Constructs and sends HTTP responses (status line, headers, body)       |
| static_handler.h & .c | Handles /static/... file serving and /sleep/... delayed responses      |
|  calc_handler.h & .c  | Handles /calc/add, /calc/mul, /calc/div operations                     |

---

## Compile the server
```bash
gcc -Wall -Wextra -pthread -o webserver webserver.c request.c response.c calc_handler.c static_handler.c
```
or
```bash
make
```

---

## Run the server
|             Goal              |                          Command                           |
| :---------------------------: | :--------------------------------------------------------: |
|  Default port 80, no verbose  |                       `./webserver`                        |
|    Chosen port, no verbose    |                  `./webserver -p <port>`                   |
| Default port 80, verbose mode |                      `./webserver -v`                      |
|   Chosen port, verbose mode   | `./webserver -p <port> -v` *or* `./webserver -v -p <port>` |

---

## Usage
### Connecting via `telnet`
In another terminal: 
```bash
telnet 127.0.0.1 <port>
GET /calc/add/5/10 HTTP/1.1
```

Ending the session:
`Ctrl + ]`, then type `quit`

### Connecting via `Postman`
New HTTP request:
```bash
GET http://127.0.0.1:<port>/calc/add/5/10
GET http://127.0.0.1:<port>/static/images/puppy.jpg
GET http://127.0.0.1:<port>/sleep/3
```

### Connecting via a web browser
In a browser:
```bash 
http://127.0.0.1:<port>/calc/add/5/10
http://127.0.0.1:<port>/static/images/puppy.jpg
http://127.0.0.1:<port>/sleep/3
```
---

## Managing the Server
### Terminating the server process
Find the process ID (PID) of the server:
```bash
lsof -i :<port>
```

Kill the process manually:
```bash
kill -9 <pid>
```

### Confirm server termination
Check if anything is still listening on the port:
```bash
lsof -i :<port>
```
It should return nothing if the server is fully terminated.

---

## Web Server Development Checklist
1. Server Setup
- [ x ] Server listens on port 80 by default.  
- [ x ] Port is changeable with -p option.  

2. HTTP Request Handling
- [ x ] Accept and parse one HTTP/1.1 request at a time.  
- [ x ] Correctly parse:  
    - [ x ] Request line (Method, Path, Version).  
    - [ x ] Headers (until empty line).  
- [ x ] Check that Method == GET.  
- [ x ] Set Content-Type appropriately based on file extension.  
- [ x ] Set Content-Length correctly in responses.  

3. Route Handling  
- [ x ] `/static` path:  
    - [ x ] Serve files from `static/` directory.  
    - [ x ] Detect and set MIME type based on file extension (e.g., .html, .jpg, .png).  
    - [ ] Prevent directory traversal attacks (e.g., using `../`).  
- [ x ] `/calc` path:
    - [ x ] Support `/calc/add/<num1>/<num2>`.  
    - [ x ] Support `/calc/mul/<num1>/<num2>`.  
    - [ x ] Support `/calc/div/<num1>/<num2>`.  
    - [ x ] Return results as basic HTML (e.g., `<html><body>Result</body></html>`).  
    - [ x ] Handle division by zero properly.  

4. Multithreading  
- [ x ] Spawn a new thread for each incoming connection.  
- [ x ] Each thread independently processes its client’s request and sends a response.  

5. Testing  
- [ x ] Test using a web browser.  
- [ x ] Test using telnet (read multiple times).  
- [ x ] Test using Postman or ThunderClient.  

6. Code Organization  
- [ x ] Create a Request module:  
    - [ x ] Functions for parsing request line and headers.  
    - [ x ] Struct for storing parsed request data.  
- [ x ] Create a Response module:  
    - [ x ] Functions for generating correct HTTP responses.  
- [ x ] Follow separation of concerns principle.  
- [ x ] Keep clean, modular interfaces between components.  

7. Extra Credit (Pipelining)  
- [ x ] Implement HTTP/1.1 pipelining:  
    - [ x ] Handle multiple client requests before sending any response.  
    - [ x ] Send responses in order of requests.  
- [ x ] Implement `/sleep/<seconds>` path to test delayed responses.  
- [ ] Use mutex locks or other synchronization to correctly manage response ordering.
