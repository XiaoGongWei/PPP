clc
clear
close all
fileNames = dir('*.txt');
lenName = numel(fileNames);
tic
for i = 1:lenName
    [num2str(i) '/' num2str(lenName) ' -> ' fileNames(i).name]
    fid = fopen(fileNames(i).name,'r');
    dX=[];dY=[];dZ=[];Num=[];
    while ~feof(fid)
        linestr = fgetl(fid);
        dNumi = linestr(37:48);
        Num = [Num str2double(dNumi)];
        linestr = linestr(48:end);
        dXi = linestr(1:11);
        dYi = linestr(13:23);
        dZi = linestr(24:35);
        dX = [dX str2double(dXi)];
        dY = [dY str2double(dYi)];
        dZ = [dZ str2double(dZi)];
    end
    fclose(fid);
%     flags = find(Num < 4);
%     dX(flags)=[];
%     dY(flags)=[];
%     dZ(flags)=[];
%     Num(flags)=[];
    h = figure('Name',fileNames(i).name);
    plot(dX,'r-')
    hold on
    plot(dY,'g-')
    hold on
    plot(dZ,'b-');
    grid on
    legend('dE','dN','dU')
    ylabel('单位(m)')
    title(['测站CUT0静态PPP定位结果趋势图'])
    %坐标轴约束
    resultVector = [dX(end - 10) dY(end - 10) dZ(end - 10)];
    minResult = min(resultVector);
    maxResult = max(resultVector);
    dmax_min = maxResult - minResult;
    deltaMaxMin = dmax_min./2;
    ylim([minResult-deltaMaxMin  maxResult+deltaMaxMin])
    %确定第几个历元收敛(后N个历元最大最小偏差绝对值在deltaM（单位m）内)
    N = 500;
    deltaM = 0.01;%2cm
    lenXYZ = length(dX);
    bestEpoch = 0;
    store_X = [];
    store_Y = [];
    store_Z = [];
    for epoch =1:lenXYZ
        if epoch + N > lenXYZ
             store_X = dX(epoch:end);
             store_Y = dY(epoch:end);
             store_Z = dZ(epoch:end);
        else
            store_X = dX(epoch:epoch+N);
            store_Y = dY(epoch:epoch+N);
            store_Z = dZ(epoch:epoch+N);
        end
        
        minX = min(store_X);
        maxX = max(store_X);
        minY = min(store_Y);
        maxY = max(store_Y);
        minZ = min(store_Z);
        maxZ = max(store_Z);
        
        bestEpoch = epoch;
        %三个方向都收敛到deltaM
        if (abs(maxX - minX) < deltaM && abs(maxY - minY) < deltaM && abs(maxZ - minZ) < deltaM)
            break;
        end

    end
    %找到收敛历元数结束---END
    line([bestEpoch bestEpoch],[minResult-deltaMaxMin maxResult+deltaMaxMin],'LineWidth',3,'Color',[1,0,0]);
    text(bestEpoch,(maxResult+minResult)/2,['收敛直线,第' num2str(bestEpoch) '历元']);
    xlabel(['历元数,收敛时间: ' num2str(bestEpoch*30/3600) 'h.' ])
    saveas(h,'CUT0定位结果.fig');
   % axis([0  3000 -5 0.5])
   %print(h,'-dpng',[fileNames(i).name '.png'])
   %close(h)
end
