%% BeforeImprovements - Per-Byte-Processing - (L)LogFiles
% Results = [RPi2BBB,BBB2RPi; RPi2BBB, BBB2RPi];
Results = [71/100, 55/99; 40/83, 49/100] * 100;

figure();
hBar = bar(Results, 'grouped');
xt = get(gca, 'XTick');
%bar(1, Results(1), 'r');
hold on;
%bar(2, Results(2), 'g');
%bar (3, Results(3), 'b');
set(gca, 'XTick', xt, 'XTickLabel', {'sleep(0.01)' 'sleep(0)'})
title("Before Any Changes-Improvements");
ylabel("Successfully Delivered Message Rates (%)");
xlabel("Tested Setups");
ylim([0 100]);
set(gca,'YGrid','on')  % horizontal grid
legend({'RPi-->BBB', 'BBB-->RPi'});
%-------------------------------------------------------------------------%

%% Per-Byte-Processing - (A)ArrayLog
% Results = [ sleep(0.01); sleep(0.001); sleep(0.0001);sleep(0)];
Results = [75/99, 80/100; 81/100, 78/95; 75/100, 73/92; 66/73, 71/100] * 100;

figure();
hBar = bar(Results, 'grouped');
xt = get(gca, 'XTick');
%bar(1, Results(1), 'r');
hold on;
%bar(2, Results(2), 'g');
%bar (3, Results(3), 'b');
set(gca, 'XTick', xt, 'XTickLabel', {'sleep(0.01)' 'sleep(0.001)' 'sleep(0.0001)' 'sleep(0)'})
title("After Improvements");
ylabel("Successfully Delivered Message Rates (%)");
xlabel("Tested Setups");
ylim([0 100]);
plot(Results(:,1));
plot(Results(:,2));
set(gca,'YGrid','on')  % horizontal grid
legend({'RPi-->BBB', 'BBB-->RPi'});
%-------------------------------------------------------------------------%

%% Per-Byte-Processing - (A)ArrayLog
% Results = [sleep(0); sleep(0.0001); sleep(0.001); sleep(0.01); sleep(0.1); sleep(1)];
Results = [67/87, 71/100; 81/96, 78/100 ;80/90, 76/100; 86/100, 62/93; 83/100, 67/100; 82/100, 48/68] * 100;

figure();
hBar = bar(Results, 'grouped');
xt = get(gca, 'XTick');
%bar(1, Results(1), 'r');
hold on;
%bar(2, Results(2), 'g');
%bar (3, Results(3), 'b');
set(gca, 'XTick', xt, 'XTickLabel', {'sleep(0)' 'sleep(0.0001)' 'sleep(0.001)' 'sleep(0.01)' 'sleep(0.1)' 'sleep(1)'})
title("After Improvements");
ylabel("Successfully Delivered Message Rates (%)");
xlabel("Tested Setups");
ylim([0 100]);
plot(Results(:,1));
plot(Results(:,2));
set(gca,'YGrid','on')  % horizontal grid
legend({'RPi-->BBB', 'BBB-->RPi'});
%-------------------------------------------------------------------------%

%% Per-Bit-Processing - (A)ArrayLog
% Results = [sleep(0.001); sleep(0.1)];
Results = [97/100, 11/87; 76/82, 12/100;] * 100;

figure(1)
hBar = bar(Results, 'grouped');
xt = get(gca, 'XTick');
%bar(1, Results(1), 'r');
hold on;
%bar(2, Results(2), 'g');
%bar (3, Results(3), 'b');
set(gca, 'XTick', xt, 'XTickLabel', {'sleep(0.001)' 'sleep(0.1)'})
title("After Improvements");
ylabel("Successfully Delivered Message Rates (%)");
xlabel("Tested Setups");
ylim([0 100]);
set(gca,'YGrid','on')  % horizontal grid
legend({'RPi-->BBB', 'BBB-->RPi'});
%-------------------------------------------------------------------------%


%% Per-Byte-Processing - (A)ArrayLog WiringPi vs BCM2835
% Results = [ sleep(0.1); sleep(0.001); sleep(0.000001);sleep(0)];
Results = [0.83333, 0.8,0.85,0.875; 0.8125, 0.65,0.777777778, 0.85; 0.83673469, 0.523809,0.8, 0.92857; 0.75, 0.3125,0.8, 0.5] * 100;
figure();
hBar = bar(Results, 'grouped');
xt = get(gca, 'XTick');
%bar(1, Results(1), 'r');
hold on;
%bar(2, Results(2), 'g');
%bar (3, Results(3), 'b');
set(gca, 'XTick', xt, 'XTickLabel', {'sleep(0.01)' 'sleep(0.001)' 'sleep(0.0001)' 'sleep(0)'})
title("WiringPi vs BCM2835");
ylabel("Successfully Delivered Message Rates (%)");
xlabel("Tested Setups");
ylim([0 100]);

%plot(Results(:,1));
%plot(Results(:,2));
%hold on;
%plot(Results(:,3));
%plot(Results(:,4));

set(gca,'YGrid','on')  % horizontal grid
legend({'RPi-->BBB(wiringPi)', 'BBB-->RPi(wiringPi)' 'RPi-->BBB(BCM2835)', 'BBB-->RPi(BCM2835)'});
%-------------------------------------------------------------------------%


%% BBB-RPi
clear
clc

successRatesBBB2RPi = 100*[3/3 7/10 3/10 2/20 0/10 2/10 1/20 0/14 0/20 1/11];
successRatesRPi2BBB = 100*[9/10 8/8 4/7 10/15 2/4 0/6 2/17 5/20 5/19 2/20];
bytesPerPacket = [1 2 4 6 8 10 15 20 25 30];
plot(bytesPerPacket, successRatesBBB2RPi);
hold all;
%plot(bytesPerPacket, successRatesRPi2BBB);
erryneg = zeros(1,10);
errypos = 100*[0/10 0 0 5/15 0/4 0/6 9/17 0/20 5/19 0/20];
errorbar(bytesPerPacket,successRatesRPi2BBB,erryneg,errypos)
title("Success Rate for used bytes per packet");
ylabel("Successfully Delivered Messages (%)");
xlabel("Used Bytes per Packet");
ylim([-5 105]);
xlim([0 31]);
legend({'BBB-->RPi', 'RPi-->BBB'});


%% RPi-RPi
clear
clc

successRatesRPiUK2RPiCH = 100*[2/2 5/5 14/16 8/14  7/20 2/18];
successRatesRPiCH2RPiUK = 100*[20/20 7/10 1/20 2/20 5/15 2/20];
bytesPerPacket = [1 2 4 6 8 30];
plot(bytesPerPacket, successRatesRPiUK2RPiCH);
hold all;
plot(bytesPerPacket, successRatesRPiCH2RPiUK);
title("Success Rate for used bytes per packet");
ylabel("Successfully Delivered Messages (%)");
xlabel("Used Bytes per Packet");
ylim([-5 105]);
xlim([0 31]);
legend({'RPiNewUK-->RPiOldCH', 'RPiOldCH-->RPiNewUK'});

