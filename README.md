# marsRover

Summary: I chose to have the 8 sensors run in a for loop until they had run for an hour and then have the threads exit so a report could be generated. 

Justification:This was the most efficient way as the separation of generating the report and the actual readings made it easy to test and also run. 

Experimentation: I tested the logic of the readings first and then I added the sleep times in and then I tested just the report generation and once I knew everything worked I put it all together and tested with many print statements. This proved to be incredibly inefficient as the testing then took extra time so I decided to retest everything sperately and lastly put it all together.

Efficieny, correctness, and prgoress guarantee: As stated before it is more efficient to separate the tasks and to have the threads exit to create the report. After much testing, it seems to run correctly.
