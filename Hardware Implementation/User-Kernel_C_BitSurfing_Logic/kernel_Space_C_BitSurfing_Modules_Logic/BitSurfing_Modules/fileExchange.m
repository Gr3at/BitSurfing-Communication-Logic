clear
close all
clc
%ReadingStep = [1000, 500, 333, 250, 200, 166, 125, 100, 83, 66, 55, 50];
ReadingStep = [100,50,25,20,15,10,9,8,7,6,5,4,3,2,1];
UsableBytesPerPacket = round(1000./ReadingStep);
bitRate = round(8*UsableBytesPerPacket/0.0418)./1000; % convert to kbps

%% ErrorRates
%{
% Before
ErrorRatesBBBRPi = 100*[0    0.0393    0.1893    0.3286    0.4905    0.4571    0.5107    0.6543    0.7071    0.6857    0.7714    0.8714];
ErrorRatesBPiRPi = 100*[0    0.0571    0.1929    0.3000    0.5357    0.4286    0.4929    0.6714    0.7429    0.7000    0.8286    0.8429];
meanDiff = mean(ErrorRatesBPiRPi-ErrorRatesBBBRPi); %meanSquaredDiff = mean((ErrorRatesBPiRPi-ErrorRatesBBBRPi).^2);
%}

% After
ErrorRatesBBBRPi = 100*[0.0714,0.1952,0.0810,0.0905,0.0714,0.0667,0.0571,0.0619,0.0952,0.1810,0.2381,0.3476,0.5524,0.8857,1];
ErrorRatesBPiRPi = 100*[0.0714,0.1905,0.0857,0.1048,0.0667,0.0238,0,0.0524,0.0810,0.2286,0.2857,0.4190,0.6714,0.6619,0.8714];
meanDiff = mean(ErrorRatesBPiRPi-ErrorRatesBBBRPi); %meanSquaredDiff = mean((ErrorRatesBPiRPi-ErrorRatesBBBRPi).^2);


figure();
plot(bitRate, ErrorRatesBBBRPi, bitRate, ErrorRatesBPiRPi, 'linewidth', 2);
%title("File Transfer Error Rates");
ylabel("Error Rate (%)");
xlabel("BitStream Data Rate (kbps)");
ylim([0 100]);
xlim([0 max(bitRate)]);
legend({'BBB-->RPi', 'BPi-->RPi'}, 'location', 'northwest');

%{ 
% smoothed plot

test1=smooth(ErrorRatesBBBRPi);
test2=smooth(ErrorRatesBPiRPi);
figure();
plot(bitRate, test1, bitRate, test2, 'linewidth', 2);
%title("File Transfer Error Rates");
ylabel("Error Rate (%)");
xlabel("BitStream Data Rate (kbps)");
ylim([0 100]);
xlim([0 max(bitRate)]);
legend({'BBB-->RPi', 'BPi-->RPi'}, 'location', 'northwest');

%}

%% time to complete test
TimeToCompleteTransfer = [8, 4.25, 1.91, 1.82, 2.66, 1.26, 0.74, 0.88, 0.61, 0.61, 0.45, 0.39]; % in hours
figure();
plot(bitRate, TimeToCompleteTransfer, 'linewidth', 2);
title("Hours to Complete File Transfer");
ylabel("Hours");
xlabel("Data Rate (bps)");
%ylim([0 max(TimeToCompleteTransfer)]);
%xlim([1 max(UsableBytesPerPacket)]);
legend({'BBB-->RPi'}, 'location', 'northwest');

%% valid Msgs
ValidMsgsChecked = (10^5)*[2.0316 2.0316    1.7872    2.0875    3.3450    2.0429    1.4142    1.6183    1.5805    1.6751    1.7487    1.6945];
ValidMsgsInterarivalTime = (10^4)* [12.4537 8.3958    6.1374    4.0369    3.0762    2.5559    2.4731    2.1046    1.9729    1.8001    1.6529    1.7630]./1000; % in microseconds
ActualMsgInterarivalTime = [183208 203054, 98315, 93656, 137116.5, 64803, 38080, 39110, 25811.5, 22960, 18667, 15923]/1000; % in miliseconds

plot(ValidMsgsInterarivalTime, ValidMsgsChecked)
figure();
plot(bitRate, log(ValidMsgsChecked), bitRate, log(ValidMsgsInterarivalTime), 'linewidth', 2);
title("Valid Messages Checked");
figure();
plot(bitRate, ValidMsgsInterarivalTime, 'linewidth', 2);
title("Valid Message Interarival Time");
figure();
plot(bitRate, ActualMsgInterarivalTime, 'linewidth', 2);
title("Actual Message Interarival Time");

