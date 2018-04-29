Dung Nguyen

Project 4 part 2b

I implement 4 test progams:

1. testcs: this test writes a new checked file and check if the checksum is correct
2. testcs2: this test read a file (README) and converts it into checked file inad check if the checksum is correct
3. testcs3: this test read the checked version of (README) and compare the text to the orginial one
NOTICE: Testcs3 must be run after testcs2 (if not there is no checked version of README)


4. testcs4: Tries to open a corrupted file

NOTICE: This test is a bit complicated to run. It need an external program (of course, by me) to corrupt the image file and reload the VM.
There are 2 ways to run this test

4.1 Run "make corrupttest" and then run "testcs4" only (please do not run other test, or you will overwrite contents of the corrupted files inside this pre-load image)


4.2 Run testcs2 first.

Then exit the VM. run "make corrupt" to use the external program to make corruption to the file. Then run the VM again.

and run "testcs4"


------------------------------------------------------------
boring parts:
- fs.c: I changed writei, readi, stati, bmap to handle the new checksum features
- sysfile.c: Change open to handle a new flag
- Several minor changes 
