Killjoy
=======

## Description
Sucks the fun out of any party by killing off the program that is passed to it.  April fools joke? No. Any good reason? Yes. Sometimes there are cases where you know a program will not give you the results you are looking for after a certain amount of time.  Use this tool to ensure that it doesn't run past the alloted time and waste CPU cycles.

## Install

Clone this repository then:

    # git clone https://github.com/ctxswitch/killjoy
    # cd killjoy
    # git checkout 1.0.1
    # make

This creates the 'killjoy' executable.  Copy the executable to it's final resting place.

## Usage

killjoy \[options\] \[utility\]

## Options

```-t, --time=[[[[D:]H:]M:]S]``` - days, hours, minutes, and seconds to run the utility

```-s, --signal=[str|int]``` - the signal to pass the child process when the limit has been reached.  This is in integer or string (KILL,TERM,etc) form.

```-q, --quiet``` - don't output any informative messages

```-h, --help``` - display the help message

## Reseting the timer

The timer can be reset by sending the USR2 signal to the killjoy process.

## Version
1.1.0
