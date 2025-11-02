# Colin MacEachern
# EECE 5520 SEC 201P
# Lab 2
# snake.py, edited due to communication issues with the arduino and due to requirements for the graduate section

# imports for libraries the game will use, pyserial is the one responsible for the COM3 serial conversation between the arduino and python script
import turtle
import time
import random
import serial

# this section enables serial comms, the COM3 is the port handling that. This section was interesting because I was having communication issues.
# Essentially, when trying to start the game often the arduino was not properly passing the wasd bytes to the python script. Research showed that there was
# methods for allowing a "handshake" between python and arduino to properly start the serial communications. This was done by first using the time.sleep(2)
# to wait for the arduino to boot.
ser = serial.Serial('COM3', 9600, timeout=0.01)
time.sleep(2)  

# Once the arduino was booted this while loop was used to wait for a READY message from the arduino, this ensured it was properly booted. Once ready is recieved
# it breaks out of the loop. This made it so the application would start properly and consistently.
while True:
    if ser.in_waiting:
        msg = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        if "READY" in msg:
            ser.reset_input_buffer()
            break

# This section is responsible for setting up some of the game variables. The scoreState variable stores the score when an apple is eaten. Initially the score per
# apple is set to 10 points. When the arduino detects a shake the scoreState is changed to 20 later on.
delay = 0.1
score = 0
high_score = 0
ppa = 10
scoreState = 10


# This section builds the window for the snake game.
wn = turtle.Screen()
wn.title("Snake Game by @TokyoEdTech (mod by YL)")
wn.bgcolor("green")
wn.setup(width=600, height=600)
wn.tracer(0)

# This section creates the actual snake.
head = turtle.Turtle()
head.speed(0)
head.shape("square")
head.color("black")
head.penup()
head.goto(0, 0)
head.direction = "stop"

# This creates the apple, the color is initally red but then is later changed to blue when a shake is detected.
food = turtle.Turtle()
food.speed(0)
food.shape("circle")
food.color("red")
food.penup()
food.goto(0, 100)

segments = []

# This section creates the trail behind the snake when an apple is eaten, and is called the "pen" in the original code
pen = turtle.Turtle()
pen.speed(0)
pen.shape("square")
pen.color("white")
pen.penup()
pen.hideturtle()
pen.goto(0, 260)
pen.write("Score: 0  High Score: 0  P/A: 10", align="center", font=("Courier", 24, "normal"))

# These are the original functions, they're defined to move where the snake is supposed to move based on data from the arduino
def go_up():
    if head.direction != "down":
        head.direction = "up"

def go_down():
    if head.direction != "up":
        head.direction = "down"

def go_left():
    if head.direction != "right":
        head.direction = "left"

def go_right():
    if head.direction != "left":
        head.direction = "right"

def move():
    if head.direction == "up":
        y = head.ycor()
        head.sety(y + 20)

    if head.direction == "down":
        y = head.ycor()
        head.sety(y - 20)

    if head.direction == "left":
        x = head.xcor()
        head.setx(x - 20)

    if head.direction == "right":
        x = head.xcor()
        head.setx(x + 20)

# Keyboard bindings for the wasd controls
wn.listen()
wn.onkey(go_up, "w")
wn.onkey(go_down, "s")
wn.onkey(go_left, "a")
wn.onkey(go_right, "d")

# Main game loop
while True:
    wn.update()

# This section was added by the student, based on the characters recieved it uses if statements and the built in functions to actually move the snake. 
# if a shake is detected the arduino sends a "z". The color and score amount is then changed
    if ser.in_waiting:
        byte = ser.read(1).decode('utf-8', errors='ignore')
        if byte == 'w': go_up()
        elif byte == 's': go_down()
        elif byte == 'a': go_left()
        elif byte == 'd': go_right()
        elif byte == 'z': 
            food.color("blue")
            scoreState = 20

     # Check for a collision with the border
    if head.xcor()>290 or head.xcor()<-290 or head.ycor()>290 or head.ycor()<-290:
        time.sleep(1)
        head.goto(0,0)
        head.direction = "stop"

        # Hide the segments
        for segment in segments:
            segment.goto(1000, 1000)
        
        # Clear the segments list
        segments.clear()

        # Reset the score
        score = 0

        # Reset the delay
        delay = 0.1

        pen.clear()
        pen.write("Score: {}  High Score: {}  P/A: {}".format(score, high_score, ppa), align="center", font=("Courier", 24, "normal")) 

    # Check for a collision with the food, the ser.write(b'E') is used to send a byte to the arduino to make the speaker beep
    if head.distance(food) < 20:

        # Move the food to a random spot
        x = random.randint(-290, 290)
        y = random.randint(-290, 290)
        food.goto(x,y)

        # Add a segment
        new_segment = turtle.Turtle()
        new_segment.speed(0)
        new_segment.shape("square")
        new_segment.color("grey")
        new_segment.penup()
        segments.append(new_segment)

        # Shorten the delay
        delay -= 0.001

        # Increase the score
        score += scoreState

        if score > high_score:
            high_score = score
        
        pen.clear()
        pen.write("Score: {}  High Score: {}  P/A: {}".format(score, high_score, ppa), align="center", font=("Courier", 24, "normal")) 
        ser.write(b'E')

    # Move the end segments first in reverse order
    for index in range(len(segments)-1, 0, -1):
        x = segments[index-1].xcor()
        y = segments[index-1].ycor()
        segments[index].goto(x, y)

    # Move segment 0 to where the head is
    if len(segments) > 0:
        x = head.xcor()
        y = head.ycor()
        segments[0].goto(x,y)

    move()    

    # Check for head collision with the body segments
    for segment in segments:
        if segment.distance(head) < 20:
            time.sleep(1)
            head.goto(0,0)
            head.direction = "stop"
        
            # Hide the segments
            for segment in segments:
                segment.goto(1000, 1000)
        
            # Clear the segments list
            segments.clear()

            # Reset the score
            score = 0

            # Reset the delay
            delay = 0.1
        
            # Update the score display
            pen.clear()
            pen.write("Score: {}  High Score: {}  P/A: {}".format(score, high_score, ppa), align="center", font=("Courier", 24, "normal")) 

    time.sleep(delay)

wn.mainloop()