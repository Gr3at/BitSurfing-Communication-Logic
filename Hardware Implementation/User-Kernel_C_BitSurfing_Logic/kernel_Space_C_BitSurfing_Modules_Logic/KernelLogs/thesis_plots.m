clear
close all
clc

% error rates for each speed (bytes used / packet) [1, 2, 3, 4, 5, 6, 7, 8, 9 ,10, 15, 20]
usedBytesPerPacket = [1, 2, 3, 4, 5];
successRate = [99/100 97/100 95/100 80/100 50/100];
errorRate = [1/100 3/100 5/100 20/100 50/100];
timeToFinish = [10000 1000 500 200 100];
timeForvalidWord = [5 1 0.5 0.1 0.05]; % in msecs

figure
plot(usedBytesPerPacket,successRate);
figure
bar(successRate);
figure
plot(usedBytesPerPacket,timeToFinish);
figure
plot(usedBytesPerPacket,timeForvalidWord); % do for all three platforms

