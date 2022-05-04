%% Import data from text file
% Script for importing data from the following text file:
%
%    filename: C:\Users\mario\Desktop\University\MECH 472\Practice\mobile_robot_modelling\mobile_robot_simulation_object\sim1.csv
%
% Auto-generated by MATLAB on 04-Apr-2022 13:09:38

%% Setup the Import Options
opts = delimitedTextImportOptions("NumVariables", 10);

% Specify range and delimiter
opts.DataLines = [1, Inf];
opts.Delimiter = ",";

% Specify column names and types
opts.VariableNames = ["time", "theta", "x", "y", "pw_r", "pw_l", "vr", "vl", "v_cmd", "w_cmd", "e_p", "e_ang"];
opts.VariableTypes = ["double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double"];
opts.ExtraColumnsRule = "ignore";
opts.EmptyLineRule = "read";

% Import the data
tbl = readtable("C:\Users\mario\Desktop\University\MECH 472\MECH_Project\Ctlr_implemented\player1_Im\sim1.csv", opts);

%% Convert to output type
time = tbl.time;
theta = tbl.theta;
x = tbl.x;
y = tbl.y;
pw_r = tbl.pw_r;
pw_l = tbl.pw_l;
vr = tbl.vr;
vl = tbl.vl;
v_cmd = tbl.v_cmd;
w_cmd = tbl.w_cmd;
e_p = tbl.e_p;
e_ang = tbl.e_ang;

%% Clear temporary variables
clear opts tbl

%% Plot


% viz = Visualizer2D;
% for i=1:2000
%     
%     viz([x(i);y(i);theta(i)]);
% 
% end

 figure
 subplot(3,3,1)
 plot(time,x)
 xlabel("Time")
 ylabel("Position X")
 
 subplot(3,3,2)
 plot(time,vr)
 xlabel("Time")
 ylabel("Vr")
 
 subplot(3,3,3)
 plot(time,v_cmd)
 xlabel("Time")
 ylabel("v_cmd")
 
 subplot(3,3,4)
 plot(time,y)
 xlabel("Time")
 ylabel("Position Y (Meters)")
 
 subplot(3,3,5)
 plot(time,vl)
 xlabel("Time")
 ylabel("Vl")
 
 subplot(3,3,6)
 plot(time,w_cmd)
 xlabel("Time")
 ylabel("w_cmd")
 
 subplot(3,3,7)
 plot(time,theta)
 xlabel("Time")
 ylabel("Orientation Theta")
 
 subplot(3,3,8)
 plot(time,e_p)
 xlabel("Time")
 ylabel("e_p")
 
 subplot(3,3,9)
 plot(time,e_ang)
 xlabel("Time")
 ylabel("e_ang")

