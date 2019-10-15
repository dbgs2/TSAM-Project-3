# P3 TSAM

## Installation

Run the make file to "install" the program

```bash
make
```

## Usage
### Running
#### Find your local ip with:

```./ip```

#### The server is run by entering the port the server should run on:

```./tsamvgroup42 <port>```

This starts the server, after starting the server enter the port the client interfaces with the server on, along with the name of the group "P3_GROUP_XX" (the name of the group is only required in debug mode)(line 825-829).

#### To connect to the client enter the server ip and server client port:

```./client <ip> <port>```

### Operating the server
The server is operated through the client, on startup the client instructs you on how to use it.

## Groups connection list (see serverMsgLog.txt)


### Inner Party 
###### (Instructors)
#### Message sent to
1. Instructor_2
1. ORACLE


#### Message receved from
1. ORACLE

### Outer Party
###### (Non-Akureyri Gropus)
#### Message sent to
1. P3_GROUP_4
1. P3_GROUP_5
1. P3_GROUP_9
1. P3_GROUP_13
1. P3_GROUP_17
1. P3_GROUP_22
1. P3_GROUP_28
1. P3_GROUP_29
1. P3_GROUP_32
1. P3_GROUP_36
1. P3_GROUP_44
1. P3_GROUP_48
1. P3_GROUP_49
1. P3_GROUP_53
1. P3_GROUP_62
1. P3_GROUP_65
1. P3_GROUP_72
1. P3_GROUP_73
1. P3_GROUP_77
1. P3_GROUP_81
1. P3_GROUP_86
1. P3_GROUP_96
1. P3_GROUP_97
1. P3_GROUP_99
1. P3_GROUP_100
1. P3_GROUP_103
1. P3_GROUP_105
1. P3_GROUP_109
1. P3_GROUP_112
1. P3_GROUP_113
1. P3_GROUP_140
1. P3_GROUP_150


#### Message receved from
1. P3_GROUP_4
1. P3_GROUP_9
1. P3_GROUP_13
1. P3_GROUP_17
1. P3_GROUP_19
1. P3_GROUP_22
1. P3_GROUP_27
1. P3_GROUP_28
1. P3_GROUP_29
1. P3_GROUP_36
1. P3_GROUP_40
1. P3_GROUP_44
1. P3_GROUP_49
1. P3_GROUP_53
1. P3_GROUP_73
1. P3_GROUP_77
1. P3_GROUP_80
1. P3_GROUP_86
1. P3_GROUP_96
1. P3_GROUP_97
1. P3_GROUP_99
1. P3_GROUP_103
1. P3_GROUP_105
1. P3_GROUP_109
1. P3_GROUP_112
1. P3_GROUP_113
1. P3_GROUP_140
1. P3_GROUP_150

### Proles
###### (Akureyri)
#### Message sent to
1. P3_GROUP_21
1. P3_GROUP_25
1. P3_GROUP_50
1. P3_GROUP_69
#### Message receved from
1. P3_GROUP_21
1. P3_GROUP_25
1. P3_GROUP_50
1. P3_GROUP_69

### Total messages sent/recived:

###### (Not including instuctors)

TOTAL: 36 sent / 32 recived

TOTAL Non-Akureyri messages: 32 sent / 28 recived

TOTAL Akureyri messages: 4 sent / 4 recived

## Oracle message
###### (Just to name a few examples)
### Encoded
1. ``` 1ed4de8986cf6e4a68d5a18c135d36f2 ```
2. ``` cb1f435c03a2e455f0e27a1c54cdf8a3 ```
3. ``` 623466d63908ff643b88d946e2f2fbd6 ```
4. ``` 31f23ba6e971170f63781be4d88b3ceb ```
5. ``` d782c260658596f870ca9a8be79f4413 ```
6. ``` 0b37d5345cdb88b60219337856bc0256 ```

### Decoded
1. ``` glue ```
2. ``` gimble ```
3. ``` baker ```
4. ``` tumtum ```
5. ``` jubjub ```
6. ``` slithy ```

### The poem
[http://www.jabberwocky.com/carroll/jabber/jabberwocky.html
](http://www.jabberwocky.com/carroll/jabber/jabberwocky.html)

### Other messages from ORACLE
See log.

## Wireshark

### GETMSG AND SENDMSG (See attached image)

### CONNECT AND LISTSERVERS (See attached image)

## Functionality

### Implemented client features

The client prints out the features implemented at startup, it's pretty straight forward.

### Implemented server features

All required features were implemented.

See attatched picture as proof that STATUSREQ/RESP works.
STATUSREQ1.png shows the following
1. Server 2 is sending a message to Server 3, Server 2 is only connected to Server 1 not Server 3.
2. Server 1 sends STATUSREQ,Server1. Gets a STATUSRESP that there is a message waiting for Server 3 and finds out that he(Server1) is connected to that server (Server 3) and sends a get message for the Server 3 message from Server 2.
3. The server (Server 1) stores the message for Server 3, and updates the KEEPALIVE.
4. Sever 1 sends KEEPALIVE to both the servers, 0 msg for Server 2 and 1 msg for Server 3.
5. Server 3 receives KEEPALIVE from Server 1. Server 3 sends GETMSG to Server 1 and finally gets the message.


## Summary
### Sent / Recive
#### Total Groups, not inst. 36/32 (1pt)
#### Akureyri 4/4 (2pt)
### Features implemented
#### All server/clients features implemented according to spec (4pt)
### Wireshark
#### All base client->server features documented. (2pt)
### Other achievements
#### Connected to the instructor server (1pt)
#### Successfully receive messages from at least 10 other groups (1pt)
#### Successfully send messages to at least 10 other groups (1pt)
#### Oracle comminications (See log) (1pt)
#### Decoded message (1pt)
#### SIngle Tar, readme ect (1pt)

### Total hours 
#### ~100-200, hard to say

### Total points 15, allegedly.