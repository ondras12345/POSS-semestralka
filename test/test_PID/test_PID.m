% Tento soubor je soucasti vyukovych materialu
% KKY/LS2, autor M. Goubej.
% Upraveno.
% Na tento soubor se NEVZTAHUJE MIT Licence, pod kterou je distribuiován zbytek
% tohoto repozitáře.
clear;
close all;

%% Model rizeneho systemu / spojity
s = zpk('s');
P = 1/(s+1)/(s+0.1)

% Diskretni ZOH model pro simulaci
global Ts;
Ts = 0.01;
Pd = c2d(P,Ts);
[n,d] = tfdata(Pd); %extrakce koeficientu pro simulaci odezvy
n=n{1};
d=d{1};

%% data z Ccka
%C = readtable('test_PID.csv')
%C.k = C.k+1

%% Parametry spojiteho PID regulatoru - paralelni forma s filtrovanou D
% slozkou
Kp = 33;
Ki = 38;
Kd = 6.6;
Tf = 0.04;
%vahy P/D slozek pro 2DoF reg.
b=.6; %P
c=0; %D
%omezeni rizeni
umax = 50;
%cas. konst. vysledovani integratoru
Tt = 5;

%Uvazovany spojity prenos reglatoru
Cs = Kp + Ki/s + Kd*s/(Tf*s+1);


%globalni promenne pro interni stav integratoru a derivatoru
global yi yd;
yi = 0; 
yd = 0;

%Minule hodnoty zp. vazby proti unaseni integratoru
global tv tvm1; 
tv = 0;
tvm1 = 0;

%Minule hodnoty param. pro bezraz. prepnuti
global Kp_old b_old Kd_old Tf_old c_old yd_old;
Kp_old = Kp;
b_old = b;
Kd_old = Kd;
Tf_old = Tf;
c_old = c;
yd_old = 0;


%% Signaly pro simulaci
n_sim = 15/Ts; %pocet kroku simulace
y = zeros(1,n_sim);
u = zeros(1,n_sim);
du = zeros(1,n_sim);
du(10/Ts:end) = 30;

w = zeros(1,n_sim);
w(5/Ts:end) = 10;
e = zeros(1,n_sim);



%% Beh simulace

for k = 3 : n_sim
    %simulace 1 kroku odezvy systemu
    y(k) = -d(2)*y(k-1)-d(3)*y(k-2)+n(2)*u(k-1)+n(3)*u(k-2);

    %Simulace zmeny parametru
    if (k>8/Ts)
        %Zmeny P slozky
        Kp = 50;
        b=1;
        
        %Zmeny D slozky
        %Kd=12;
        c=1;
        %Tf=Tf/3;
        %w(k) = 11;
    end
    
    
    %volani funkce s vypoctem PID regulatoru + vstupni porucha
    
    %u(k) = calc_pid(Kp,Ki,Kd,Tf,w(k),w(k-1),y(k),y(k-1)); %1DoF
    %u(k) = calc_pid2(Kp,Ki,Kd,Tf,w(k),w(k-1),y(k),y(k-1),b,c); %2DoF
    %u(k) = calc_pid3(Kp,Ki,Kd,Tf,w(k),w(k-1),y(k),y(k-1),b,c,umax,Tt); %2DoF + AWU
    u(k) = calc_pid4(Kp,Ki,Kd,Tf,w(k),w(k-1),w(k-2),y(k),y(k-1),y(k-2),b,c,umax,Tt); %2DoF + AWU  + bezraz
    %u(k) = calc_pid4_2(Kp,Ki,Kd,Tf,w(k),w(k-1),w(k-2),y(k),y(k-1),y(k-2),b,c,umax,Tt); %2DoF + AWU  + bezraz - jednodussi implementace (nemusim archivovat stare parametry)
    

    %omezeni akcniho zasahu - saturace
    if abs(u(k)) > umax
        u(k) = umax*sign(u(k));
    end

    %pridani vstupni poruchy
    u(k) = u(k) + du(k);

end


%%% Vykresleni vysledku
%t=[0:1:n_sim-1]*Ts;
%
%figure(1)
%clf
%%step(Pd)
%%hold on;
%stairs(t, y,'r', 'DisplayName', 'y MATL')
%hold on
%stairs(t, C.y, 'DisplayName', 'y C')
%xlabel('cas [s]')
%ylabel('vystup y(kT)')
%grid on;
%legend
%
%figure(2)
%clf
%stairs(t,u-du, 'DisplayName', 'u MATL');
%hold on;
%stairs(t,du, 'DisplayName', 'du MATL');
%stairs(t, C.u-C.du, ':', 'LineWidth', 1.5, 'DisplayName', 'u C')
%stairs(t, C.du, ':', 'LineWidth', 1.5, 'DisplayName', 'du C')
%xlabel('cas [s]')
%ylabel('rizeni u(kT), porucha du(kT)')
%legend
%grid on;

%% ulozeni vysledku
M = table();
M.k = (0:n_sim-1)';
M.u = u';
M.y = y';
M.du = du';
M
writetable(M, 'test_PID_MATLAB.csv', 'Delimiter', '\t')

%% Kod lokalni funkce pro vypocet PID regulatoru

% Zakladni 1DoF verze
function uk = calc_pid(Kp,Ki,Kd,Tf,wk,wkm1,yk,ykm1)
    global yi yd Ts;
    
    %Fyzikalni parametry PID na interni diskretni zesileni
    ci=Ki*Ts/2; 
    cd1=-(Ts-2*Tf)/(Ts+2*Tf); 
    cd2=2*Kd/(Ts+2*Tf);
    
    %Reg. odchylka
    ek = wk - yk;
    ekm1 = wkm1 - ykm1;
    
    %Akcni zasah po slozkach
    yi = yi + ci*(ek+ekm1);
    yd = cd1*yd + cd2*(ek-ekm1);
    yp = Kp*ek;

    uk = yp+yi+yd; %celkovy aktualni akcni zasah - vystup regulatoru

end

% 2DoF reg.
function uk = calc_pid2(Kp,Ki,Kd,Tf,wk,wkm1,yk,ykm1,b,c)
    global yi yd Ts;
    
    %Fyzikalni parametry PID na interni diskretni zesileni
    ci=Ki*Ts/2; 
    cd1=-(Ts-2*Tf)/(Ts+2*Tf); 
    cd2=2*Kd/(Ts+2*Tf);
    
    %Reg. odchylka pro I slozku
    ek = wk - yk;
    ekm1 = wkm1 - ykm1;
    
    %Vazena odchylka pro P slozku
    ep = b*wk - yk;
    
    %Vazena odchylka pro D slozku
    ed = c*wk - yk;
    edm1 = c*wkm1 - ykm1;

    yi = yi + ci*(ek+ekm1);
    yd = cd1*yd + cd2*(ed-edm1);
    yp = Kp*ep;

    uk = yp+yi+yd; %aktualni akcni zasah - vystup regulatoru
end

% 2DoF reg. + saturace + anti-windup
function uk = calc_pid3(Kp,Ki,Kd,Tf,wk,wkm1,yk,ykm1,b,c,umax,Tt)
    global yi yd Ts tv tvm1;
    
    %Fyzikalni parametry PID na interni diskretni zesileni
    ci=Ki*Ts/2; 
    cd1=-(Ts-2*Tf)/(Ts+2*Tf); 
    cd2=2*Kd/(Ts+2*Tf);
    
    %Reg. odchylka pro I slozku
    ek = wk - yk - tv;
    ekm1 = wkm1 - ykm1 - tvm1;
    
    %Vazena odchylka pro P slozku
    ep = b*wk - yk;
    
    %Vazen cxza\a odchylka pro D slozku
    ed = c*wk - yk;
    edm1 = c*wkm1 - ykm1;

    yi = yi + ci*(ek+ekm1);
    yd = cd1*yd + cd2*(ed-edm1);
    yp = Kp*ep;

    u_nosat = yp+yi+yd; %aktualni akcni zasah - vystup regulatoru
    
    if abs(u_nosat)>umax
        u_sat = umax*sign(u_nosat);
        tvm1 = tv;
        tv = (u_nosat - u_sat)/Tt;
        uk = u_sat;
    else
        tvm1 = tv;
        tv = 0;
        uk = u_nosat;
    end

end



% 2DoF reg. + saturace + anti-windup + bezraz. prepinani
function uk = calc_pid4(Kp,Ki,Kd,Tf,wk,wkm1,wkm2,yk,ykm1,ykm2,b,c,umax,Tt)
    global yi yd Ts tv tvm1 Kp_old b_old Kd_old c_old Tf_old;
    
    %Fyzikalni parametry PID na interni diskretni zesileni
    ci=Ki*Ts/2; 
    cd1=-(Ts-2*Tf)/(Ts+2*Tf); 
    cd2=2*Kd/(Ts+2*Tf);
    
    
    %Offset stare hodnoty integrator pro kompenzaci razu vlivem zmeny
    %parametru - P-slozka
    yi = yi -( Kp*(b*wkm1-ykm1)-Kp_old*(b_old*wkm1-ykm1) );
    %yi = yi -( Kp*(b*wk-yk)-Kp_old*(b_old*wk-yk) ); %MOHLO BY BYT
    %TEORETICKY Z AKTUALNICH VZORKU, ALE DOSLO BY K BLOKOVANI P-SLOZKY PRI
    %ZMENE PARAMETRU I POZADOVANE HODNOTY V JEDNOM KROKU
        
        
    %D - slozka
    cd1_old=-(Ts-2*Tf_old)/(Ts+2*Tf_old); 
    cd2_old=2*Kd_old/(Ts+2*Tf_old);
    
    yi = yi -( cd1*yd + cd2*(c*wkm1-ykm1-(c*wkm2-ykm2) ) - (cd1_old*yd + cd2_old*(c_old*wkm1-ykm1-(c_old*wkm2-ykm2))) ); 
    
    
    
    %Reg. odchylka pro I slozku
    ek = wk - yk - tv;
    ekm1 = wkm1 - ykm1 - tvm1;
    
    %Vazena odchylka pro P slozku
    ep = b*wk - yk;
    
    
    
    
    
    %Vazen cxza\a odchylka pro D slozku
    ed = c*wk - yk;
    edm1 = c*wkm1 - ykm1;

    yi = yi + ci*(ek+ekm1);
    yd = cd1*yd + cd2*(ed-edm1);
    yp = Kp*ep;

    u_nosat = yp+yi+yd; %aktualni akcni zasah - vystup regulatoru
    
    if abs(u_nosat)>umax
        u_sat = umax*sign(u_nosat);
        tvm1 = tv;
        tv = (u_nosat - u_sat)/Tt;
        uk = u_sat;
    else
        tvm1 = tv;
        tv = 0;
        uk = u_nosat;
    end
    
    %Ulozeni pouzitych parametru
    Kp_old = Kp;
    b_old = b;
    Kd_old = Kd;
    Tf_old = Tf;
    c_old = c;

end



% 2DoF reg. + saturace + anti-windup + bezraz. prepinani
% Implementace bez nutnosti archivace starych parametru
function uk = calc_pid4_2(Kp,Ki,Kd,Tf,wk,wkm1,wkm2,yk,ykm1,ykm2,b,c,umax,Tt)
    global yi yd yd_old Ts tv tvm1;
    
    %Fyzikalni parametry PID na interni diskretni zesileni
    ci=Ki*Ts/2; 
    cd1=-(Ts-2*Tf)/(Ts+2*Tf); 
    cd2=2*Kd/(Ts+2*Tf);
    
    
    %Offset stare hodnoty integrator pro kompenzaci razu vlivem zmeny
    %parametru - P-slozka
    %yi = yi -( Kp*(b*wkm1-ykm1)-Kp_old*(b_old*wkm1-ykm1) );
    yi = yi - Kp*(b*wkm1-ykm1); %treti clen muzu pricist na konci cyklu
    
           
    %D - slozka
    %cd1_old=-(Ts-2*Tf_old)/(Ts+2*Tf_old); 
    %cd2_old=2*Kd_old/(Ts+2*Tf_old);
    
    %yi = yi -( cd1*yd + cd2*(c*wkm1-ykm1-(c*wkm2-ykm2) ) - (cd1_old*yd + cd2_old*(c_old*wkm1-ykm1-(c_old*wkm2-ykm2))) ); 
    yi = yi - ( cd1*yd_old + cd2*(c*wkm1-ykm1-(c*wkm2-ykm2) )); %treti clen prictu na konci cyklu
    
    
    %Reg. odchylka pro I slozku
    ek = wk - yk - tv;
    ekm1 = wkm1 - ykm1 - tvm1;
    
    %Vazena odchylka pro P slozku
    ep = b*wk - yk;
    
        
    %Vazen odchylka pro D slozku
    ed = c*wk - yk;
    edm1 = c*wkm1 - ykm1;
    yd_old = yd;
    
    yi = yi + ci*(ek+ekm1);
    yd = cd1*yd + cd2*(ed-edm1);
    yp = Kp*ep;

    u_nosat = yp+yi+yd; %aktualni akcni zasah - vystup regulatoru
    
    if abs(u_nosat)>umax
        u_sat = umax*sign(u_nosat);
        tvm1 = tv;
        tv = (u_nosat - u_sat)/Tt;
        uk = u_sat;
    else
        tvm1 = tv;
        tv = 0;
        uk = u_nosat;
    end
    
    %Ulozeni pouzitych parametru
    %Kp_old = Kp;
    
    %yi = yi -( Kp*(b*wkm1-ykm1)-Kp_old*(b_old*wkm1-ykm1) ); %z predchozi
    %varianty implementace
    yi = yi + yp; %aktualni cas. index, promitne se az do dalsiho kroku, cimz se z wk a yk stanou 1 krok stare hodnoty
    yi = yi + yd;
    
    
    

end
