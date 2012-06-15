
#ifndef CONTROL_H__
#define CONTROL_H__

void setDesiredHeading(int heading);
void setDesiredDepth(int depth);
void setDesiredPower(int power);

extern int _desired_heading;
extern int _desired_depth;
extern int _desired_power;

extern int _current_heading;
extern int _current_depth;
extern int _current_power;

#endif