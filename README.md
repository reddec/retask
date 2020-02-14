## Architecture 

* Worker - is a entity that is processing one-by-one tasks for the same processor.
* Processor - (also: executable, app) executable file (language agnostic) that getting input from STDIN 
and returning result to STDOUT with optional logs to STDERR. Return code 0 means success, otherwise - fail.
* Task - single processing unit that contains saved file as STDIN for the processor

> Briefly - worker gets file from `tasks` directory, feeds it to STDIN of executable.
>
> On success (ret code 0) - saves STDOUT to result dir and remove task file.
>
> On fail - moves task file to `requeue` directory. It will be later moved back to `tasks` directory

Logic is quite straight forward and could be described by one diagram

![flow](https://user-images.githubusercontent.com/6597086/74507235-c464f380-4f36-11ea-8237-0665c7096a81.png)
 
Task states
 
![states](https://user-images.githubusercontent.com/6597086/74504852-7ea52c80-4f30-11ea-9487-b11e383f26c6.png)

## Why C ?

I know Go, Java, Python, C#, JavaScript, C++ and number of another languages. I could write the project on any of
them and highly likely it will be much faster, much easier for newcomers and easier to maintain.

But the project is on C...

* I like C for it's simplicity and minimalistic syntax, low memory consumption
* It is fun to code something like this on low-level language like C: needs to remember a lot of stuff
* It is kind of respect to Linux/POSIX systems
* It is a hobby project, so there is no business requirements ;-)

## TODO

* Project-based - for each directory in the running directory worker should be run
* Publish to repositories
* Cross-compile for ARM