% plots

%% same #messages different network size
successfulMsgs = [10 10 8 8 6 7 6 5 5 4]./10*100;
networkSize = [2 4 6 8 12 16 20 24 30 32];

smoothSuccessMsgs = smooth(successfulMsgs);

h=plot(networkSize, smoothSuccessMsgs);
set(gca, 'XTick',networkSize(1:end), 'XTickLabel',networkSize(1:end))
h.LineWidth =2;
h.Color = [0.1 0.7 0.8];
xlim([0,max(networkSize)+1])
ylim([0, max(successfulMsgs)+1])
ylabel("Successfully Delivered Messages (%)");
xlabel("Network Size (#nodes)");


%% same network size different #messages
successfulMsgs = [1 4 9 7 8 15 10 12 25 29 28 44];
totalMsgsSent = [1 5 10 20 30 40 50 60 70 80 90 100];

tmp= (successfulMsgs./totalMsgsSent)*100;
smoothSuccessMsgs = smooth(tmp);

h=plot(totalMsgsSent, smoothSuccessMsgs);
set(gca, 'XTick',totalMsgsSent(1:end), 'XTickLabel',totalMsgsSent(1:end))
h.LineWidth =2;
h.Color = [0.1 0.7 0.8];
xlim([0,max(totalMsgsSent)+1])
ylim([0, max(smoothSuccessMsgs)+1])
ylabel("Successfully Delivered Messages (%)");
xlabel("Total Number of Messages Sent (#)");

%%
AverageBitsToDelivery = [34964.4 49111.9 51706.25 76219.88 122698 104197.85 72562.5 63569.8 214691.8 236926.5]./1000;
networkSize = [2 4 6 8 12 16 20 24 30 32];

smoothSuccessMsgs = smooth(AverageBitsToDelivery);

h=plot(networkSize, smoothSuccessMsgs);
set(gca, 'XTick',networkSize(1:end), 'XTickLabel',networkSize(1:end))
h.LineWidth =2;
h.Color = [0.1 0.7 0.8];
xlim([0,max(networkSize)+1])
ylim([0, max(AverageBitsToDelivery)+1])
ylabel("Average #kbits Required from end to end");
xlabel("Network Size (#nodes)");
