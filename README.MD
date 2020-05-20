# Proiect: mySvn
## Sumanaru Catalin
## Universitatea Alexandru Ioan Cuza
## Facultatea de Informatica
## Grupa A2

### Installation:

``` 
$make ------ Compileaza toate executabilele necesare.
$make install 	-------- Copiaza fisierele necesare (cli & server) in /usr/local/bin
```

### Usage:

##### Starting the Server:
$mysvnserver

##### Config setup:

$mysvn set port 2908
$mysvn set ip 127.0.0.1


##### Register:
$mysvn register  ---- va porni un wizzard pentru creearea unui cont


##### Login (optional):
$mysvn set username <<username>>

##### Repo initialization:
$mysvn init

##### Cloning repo:
$mysvn clone <<repo_id>> (optained by running info)

##### Repo information and permissions:
$mysvn info

##### Commit
$mysvn commit

##### Revert
$mysvn revert <<version>> 
(must be between 1 and <<latest_version>>)

##### Checkout
$mysvn checkout <<version>> 
(must be between 1 and <<latest_version>>)


##### Keeping track of files:

By default all files are kept track off as they are added.

To ignore a file: $mysvn rm <<file>>
To keep track again: $mysnv add <<file>>

##### Adding or changing permissions to users:

$mysvn permissions <read/write/modify/erase> <true/false> <username>
