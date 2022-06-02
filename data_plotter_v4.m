%edits: 
% revert to temp2_diff = amb_temp - samp2_temp; #done

data = Cotton1PadOutdoors;
plot_title = "Cotton 1 Pad Outdoors 20220428";
maperiod = 15*60; %moving average period in seconds so 15*60 = 900s(15mins)
n = height(data);
ps = maperiod+1; %plot start
zeros1(1:n) = 0;
disp(data(1:1,:));

%{
x_sec = data(:,6);
samp1_temp = data(:,3);
samp2_temp = data(:,4);
amb_temp = data(:,5);
energy_tot = data(:,7);
energy_in = data(:,8);
hpower = data(:,9);
temp1_diff(1:n) = 0;
temp2_diff(1:n) = 0;
for i = 1:n
    temp1_diff(i) = amb_temp(i,1) - samp1_temp(i,1);
    temp2_diff(i) = amb_temp(i,1) - samp2_temp(i,1);
end
temp1_diff;
temp2_diff
%}

dtime = data.Date_Time;
samp1_temp = data.S1_TempC;
samp2_temp = data.S2_TempC;
amb_temp = data.Amb_TempC;
x_sec = data.e_times;
energy_tot = data.energy_totJ;
energy_in = data.energy_inJ;
hpower = data.hpowerW;
temp1_diff = amb_temp - samp1_temp;
temp2_diff = amb_temp - samp2_temp;
temp12_diff = samp1_temp - samp2_temp;

samp1_tempma(1:n) = 0;
samp2_tempma(1:n) = 0;
amb_tempma(1:n) = 0;
hpowerma(1:n) = 0;
temp2_diffma(1:n) = 0;
temp1_diffma(1:n) = 0;
temp12_diffma(1:n) = 0;
%{
for j = 2:n     %gets rid of extreme values(outliers/errors), tune to each data set
    if samp2_temp(j) > 50 || samp2_temp(j) < 2
    %if samp2_temp(j) > (samp1_temp(j)+200) || samp2_temp(j) < (samp1_temp(j)-20)
        samp2_temp(j) = samp2_temp(j-1);
        temp2_diff(j) = temp2_diff(j-1);
    end
end
%}
for j = maperiod+1:n    %smoothes the data
    samp1_tempma(j) = mean(samp1_temp(j-maperiod:j));
    samp2_tempma(j) = mean(samp2_temp(j-maperiod:j));
    amb_tempma(j) = mean(amb_temp(j-maperiod:j));
    hpowerma(j) = mean(hpower(j-maperiod:j));
    temp2_diffma(j) = mean(temp2_diff(j-maperiod:j));
    temp1_diffma(j) = mean(temp1_diff(j-maperiod:j));
    temp12_diffma(j) = mean(temp12_diff(j-maperiod:j));
end


subplot(5,1,1)
plot(dtime(ps:n),samp2_tempma(ps:n),"b",'DatetimeTickFormat','hh:mm a')
hold on
plot(dtime(ps:n),amb_tempma(ps:n),"r",'DatetimeTickFormat','hh:mm a')
hold off
ylabel("Temperature (°C)")
xlabel("Time")
title(plot_title)
legend('Sample 2 Temperature','Ambient Temperature','location','best')

subplot(5,1,2)
plot(dtime(ps:n),samp1_tempma(ps:n),"b",'DatetimeTickFormat','hh:mm a')
hold on
plot(dtime(ps:n),amb_tempma(ps:n),"r",'DatetimeTickFormat','hh:mm a')
hold off
ylabel("Temperature (°C)")
xlabel("Time")
legend('Sample 1 Temp(heated)','Ambient Temperature','location','best')
%{
subplot(4,1,2)
%plot(dtime(1:n),energy_tot(1:n),'DatetimeTickFormat','hh:mm a')
%hold on
plot(dtime(1:n),energy_in(1:n),'DatetimeTickFormat','hh:mm a')
%hold off
ylabel("Energy Supplied (J)")
xlabel("Time")
%legend('Cummulative Energy','Energy','location','southeast')
%}
%{
subplot(4,1,3)
plot(dtime(ps:n),hpowerma(ps:n)*1000,'DatetimeTickFormat','hh:mm a')
ylabel("Heating Power(mW)")
xlabel("Time")
%}
subplot(5,1,3)  
plot(dtime(ps:n),temp2_diffma(ps:n),'DatetimeTickFormat','hh:mm a')
hold on
plot(dtime(ps:n),zeros1(ps:n),"k --",'DatetimeTickFormat','hh:mm a')
hold off
ylabel("Temp. Diff T2(°C)")
xlabel("Time")

subplot(5,1,4)  
plot(dtime(ps:n),temp1_diffma(ps:n),'DatetimeTickFormat','hh:mm a')
hold on
%plot(dtime(ps:n),zeros1(ps:n),"k --",'DatetimeTickFormat','hh:mm a')
hold off
ylabel("Temp. Diff T1(°C)")
xlabel("Time")

subplot(5,1,5)  
plot(dtime(ps:n),temp12_diffma(ps:n),'DatetimeTickFormat','hh:mm a')
hold on
plot(dtime(ps:n),zeros1(ps:n),"k --",'DatetimeTickFormat','hh:mm a')
hold off
ylabel("Temp. Diff T1-T2(°C)")
xlabel("Time")

saveas(gcf,plot_title);  %saves figure as matlab fig
saveas(gcf,plot_title,'png');    %saves figure as png
save(plot_title);        %saves all variables from current workspace

format bank;
disp(plot_title)
disp(['     Avg AmbTemp(°C)', '  Max CPow(mW)', '  Avg Cpow(mW)'])
disp([mean(amb_temp), max(hpowerma)*1000, mean(hpower)*1000]);
format default;

%plot(temp2_diffma,hpowerma,'.');xlabel('temp2 diff');ylabel('hpower') 
%relationship btw temp diff and heating power