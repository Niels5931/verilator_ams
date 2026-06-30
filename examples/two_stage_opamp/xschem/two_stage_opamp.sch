v {xschem version=3.4.7 file_version=1.2}
G {}
K {}
V {}
S {}
E {}
N 510 -300 530 -300 {lab=Vp}
N 270 -300 290 -300 {lab=Vn}
N 370 -190 430 -190 {lab=#net1}
N 400 -240 400 -190 {lab=#net1}
N 470 -270 470 -220 {lab=VO}
N 330 -240 400 -240 {lab=#net1}
N 470 -250 560 -250 {lab=VO}
N 330 -120 400 -120 {lab=GND}
N 400 -120 470 -120 {lab=GND}
N 470 -160 470 -120 {lab=GND}
N 330 -360 450 -360 {lab=#net2}
N 330 -360 330 -330 {lab=#net2}
N 450 -360 470 -360 {lab=#net2}
N 470 -360 470 -330 {lab=#net2}
N 400 -120 400 -80 {lab=GND}
N 400 -380 400 -370 {lab=#net2}
N 400 -370 400 -360 {lab=#net2}
N 150 -380 150 -240 {lab=BIAS}
N 150 -480 150 -440 {lab=VDD}
N 150 -480 400 -480 {lab=VDD}
N 400 -480 400 -440 {lab=VDD}
N 260 -550 260 -480 {lab=VDD}
N 510 -300 530 -300 {lab=Vp}
N 270 -300 290 -300 {lab=Vn}
N 470 -270 470 -220 {lab=VO}
N 330 -360 330 -330 {lab=#net2}
N 470 -360 470 -330 {lab=#net2}
N 330 -240 330 -220 {lab=#net1}
N 330 -270 330 -240 {lab=#net1}
N 330 -160 330 -120 {lab=GND}
N 110 -410 110 -360 {lab=BIAS}
N 110 -360 150 -360 {lab=BIAS}
N 150 -360 260 -360 {lab=BIAS}
N 260 -410 260 -360 {lab=BIAS}
N 260 -410 360 -410 {lab=BIAS}
C {sky130_fd_pr/pfet3_01v8_lvt.sym} 380 -410 0 0 {name=M3
W=12
L=1
body=VDD
nf=1
mult=1
ad="expr('int((@nf + 1)/2) * @W / @nf * 0.29')"
pd="expr('2*int((@nf + 1)/2) * (@W / @nf + 0.29)')"
as="expr('int((@nf + 2)/2) * @W / @nf * 0.29')"
ps="expr('2*int((@nf + 2)/2) * (@W / @nf + 0.29)')"
nrd="expr('0.29 / @W ')" nrs="expr('0.29 / @W ')"
sa=0 sb=0 sd=0
model=pfet_01v8_lvt
spiceprefix=X
}
C {sky130_fd_pr/nfet3_01v8_lvt.sym} 450 -190 0 0 {name=M4
W=10
L=3
body=GND
nf=1
mult=1
ad="expr('int((@nf + 1)/2) * @W / @nf * 0.29')"
pd="expr('2*int((@nf + 1)/2) * (@W / @nf + 0.29)')"
as="expr('int((@nf + 2)/2) * @W / @nf * 0.29')"
ps="expr('2*int((@nf + 2)/2) * (@W / @nf + 0.29)')"
nrd="expr('0.29 / @W ')" nrs="expr('0.29 / @W ')"
sa=0 sb=0 sd=0
model=nfet_01v8_lvt
spiceprefix=X
}
C {sky130_fd_pr/nfet3_01v8_lvt.sym} 350 -190 0 1 {name=M5
W=10
L=3
body=GND
nf=1
mult=1
ad="expr('int((@nf + 1)/2) * @W / @nf * 0.29')"
pd="expr('2*int((@nf + 1)/2) * (@W / @nf + 0.29)')"
as="expr('int((@nf + 2)/2) * @W / @nf * 0.29')"
ps="expr('2*int((@nf + 2)/2) * (@W / @nf + 0.29)')"
nrd="expr('0.29 / @W ')" nrs="expr('0.29 / @W ')"
sa=0 sb=0 sd=0
model=nfet_01v8_lvt
spiceprefix=X
}
C {iopin.sym} 400 -80 0 0 {name=gnd_i lab=GND}
C {ipin.sym} 150 -240 0 0 {name=bias_i lab=BIAS}
C {ipin.sym} 530 -300 2 0 {name=Vp_i lab=Vp}
C {ipin.sym} 270 -300 0 0 {name=Vn_i lab=Vn}
C {opin.sym} 560 -250 0 0 {name=Vo lab=VO}
C {sky130_fd_pr/pfet3_01v8_lvt.sym} 130 -410 0 0 {name=M6
W=10
L=1
body=VDD
nf=1
mult=1
ad="expr('int((@nf + 1)/2) * @W / @nf * 0.29')"
pd="expr('2*int((@nf + 1)/2) * (@W / @nf + 0.29)')"
as="expr('int((@nf + 2)/2) * @W / @nf * 0.29')"
ps="expr('2*int((@nf + 2)/2) * (@W / @nf + 0.29)')"
nrd="expr('0.29 / @W ')" nrs="expr('0.29 / @W ')"
sa=0 sb=0 sd=0
model=pfet_01v8_lvt
spiceprefix=X
}
C {iopin.sym} 260 -550 0 0 {name=VDD_i1 lab=VDD}
C {sky130_fd_pr/pfet3_01v8_lvt.sym} 310 -300 0 0 {name=M1
W=35
L=5
body=VDD
nf=2
mult=4
ad="expr('int((@nf + 1)/2) * @W / @nf * 0.29')"
pd="expr('2*int((@nf + 1)/2) * (@W / @nf + 0.29)')"
as="expr('int((@nf + 2)/2) * @W / @nf * 0.29')"
ps="expr('2*int((@nf + 2)/2) * (@W / @nf + 0.29)')"
nrd="expr('0.29 / @W ')" nrs="expr('0.29 / @W ')"
sa=0 sb=0 sd=0
model=pfet_01v8_lvt
spiceprefix=X
}
C {sky130_fd_pr/pfet3_01v8_lvt.sym} 490 -300 0 1 {name=M7
W=35
L=5
body=VDD
nf=2
mult=4
ad="expr('int((@nf + 1)/2) * @W / @nf * 0.29')"
pd="expr('2*int((@nf + 1)/2) * (@W / @nf + 0.29)')"
as="expr('int((@nf + 2)/2) * @W / @nf * 0.29')"
ps="expr('2*int((@nf + 2)/2) * (@W / @nf + 0.29)')"
nrd="expr('0.29 / @W ')" nrs="expr('0.29 / @W ')"
sa=0 sb=0 sd=0
model=pfet_01v8_lvt
spiceprefix=X
}
