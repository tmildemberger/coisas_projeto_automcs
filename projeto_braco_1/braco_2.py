import cadquery as cq
from cadquery import exporters
from cadquery.selectors import BoxSelector
from motor_2 import motor_2, motor_2_dim
from testes_acoplamento_motor_2 import acoplamento_motor_2

raio_arredondamentos = 1.6 / 2

profundidade_acoplamento = 7.2
profundidade_base_acoplamento = 2.8

# em milímetros
comprimento_braco = 95 - raio_arredondamentos
largura_braco = 12 - 2 * raio_arredondamentos
altura_braco = profundidade_acoplamento + profundidade_base_acoplamento - 2 * raio_arredondamentos

espacamento_base_acoplamento = 0.8

comprimento_prender_ima = 24
raio_ondulacao = 1

raio_base_acoplamento_2 = 10 - raio_arredondamentos


local_ima = 89
raio_ima = 25 / 2
altura_ima = 20

w_ima = cq.Workplane('XY', origin=(0, local_ima, altura_braco + 2*raio_arredondamentos)).circle(raio_ima).extrude(altura_ima)
#show_object(w_ima)

diff_sup = 0.04
r_sup = raio_arredondamentos + diff_sup
w_sup = cq.Workplane('XY', origin=(0, 0, altura_braco + 2*r_sup)).moveTo(largura_braco/2 + diff_sup, comprimento_braco - r_sup)#.circle(0.2).extrude(2)
p = comprimento_braco - r_sup
o = 1
for i in range(7):
    w_sup = w_sup.threePointArc((largura_braco/2 - raio_ondulacao*o + diff_sup, p - (raio_ondulacao-o*diff_sup)), (largura_braco/2 + diff_sup, p - 2*(raio_ondulacao-o*diff_sup)))
    p -= 2*(raio_ondulacao-o*diff_sup)
    o = -1 * o
    
#def sweep_func(t):
    #if t >= 0 and t < 3:
        #return (0.02, 0, -t)
    #elif t >= 3 and t < 6:
        #return ((t-2)*0.02, 0, -t)
    #elif t >= 6 and t <= 9:
        #return (1.08 - ((1-(t-6)/3)^(1/4)), 0, -t)
        
def sweep_func(t):
    if t >= 0 and t < 3:
        return (0.0, 0, -t)
    elif t >= 3 and t < 6:
        return ((t-3)*0.02, 0, -t)
    elif t >= 6 and t <= 9:
        return (1.06 - ((1-(t-6)/3)**(1/4)), 0, -t)

sweep_path = cq.Workplane('XZ').moveTo(0, 0)
sweep_path = sweep_path.lineTo(0, -3)
#sweep_path = sweep_path.lineTo(3, -3)
#sweep_path = sweep_path.lineTo(0, 0).close().extrude(1)
sweep_path = sweep_path.tangentArcPoint((0.04, -6), relative=False)
sweep_path = sweep_path.ellipseArc(2, 3, 180, 270, 0, 1)
#sweep_path = sweep_path.close().extrude(1)

#p_sweep = cq.Workplane('XY').parametricCurve(sweep_func, 10, 0, 9)
w_sup = (w_sup.line(8, 0).line(0, 14*raio_ondulacao-2*diff_sup).line(-8, 0).close()
         .sweep(sweep_path, True, normal=(0, 0, 1)).translate((0.02, 0, 0)))#.extrude(2))
w_sup = w_sup.mirror('YZ', union=True)
#w_sup = w_sup.union(cq.Workplane('XY', origin=(0, local_ima, altura_braco + 2*r_sup)).box(largura_braco + 2*r_sup+4, 10*raio_ondulacao-2*diff_sup, 2, (True, True, False)))
w_sup = w_sup.union(cq.Workplane('XY', origin=(0, local_ima, altura_braco + 2*r_sup)).circle(raio_ima).extrude(7))
w_sup = w_sup.intersect(cq.Workplane('XY', origin=(0, local_ima, altura_braco + 2*r_sup)).circle(raio_ima).extrude(15, both=True))
w_sup = w_sup.edges('%circle and >Z').fillet(0.2)
w_sup = w_sup.edges('|X').fillet(raio_arredondamentos-2*diff_sup)
w_sup = w_sup.edges('%circle and >>Z[-5]').fillet(0.2)
#show_object(sweep_path)

raio_parafuso = 4 / 2
raio_cabeca= 8 / 2
altura_cabeca = 5
w_sup = w_sup.cut(cq.Workplane('XY', origin=(0, local_ima, 0)).circle(raio_parafuso+3*diff_sup).extrude(40, both=True))
w_sup = w_sup.cut(cq.Workplane('XY', origin=(0, local_ima, altura_braco + 2*r_sup + altura_cabeca)).circle(raio_cabeca+3*diff_sup).extrude(-3*altura_cabeca))
show_object(w_sup)
#show_object(w_sup.edges('%circle and >>Z[-5]'))#.edges('|X'))

motor_2_dim = motor_2_dim()
w_motor = motor_2((motor_2_dim.raio - 6.5, 0, -motor_2_dim.altura -9.5 + profundidade_acoplamento))

show_object(w_motor)

acoplamento = acoplamento_motor_2(profundidade_acoplamento, profundidade_acoplamento + profundidade_base_acoplamento, True)

show_object(acoplamento)

w = cq.Workplane('XY', origin=(0, 0, espacamento_base_acoplamento)).moveTo(-raio_base_acoplamento_2, 0).threePointArc((0, -raio_base_acoplamento_2), (raio_base_acoplamento_2, 0))
w = w.lineTo(largura_braco / 2, largura_braco).lineTo(largura_braco / 2, comprimento_braco - comprimento_prender_ima)
p = comprimento_braco - comprimento_prender_ima
o = 1
for i in range(comprimento_prender_ima // (2*raio_ondulacao)):
    w = w.threePointArc((largura_braco / 2 + raio_ondulacao*o - raio_arredondamentos, p + (raio_ondulacao-o*raio_arredondamentos)), (largura_braco / 2, p + 2*(raio_ondulacao-o*raio_arredondamentos)))
    p += 2*(raio_ondulacao-o*raio_arredondamentos)
    o = -1 * o

w = w.threePointArc((0, comprimento_braco + largura_braco/2), (-largura_braco/2, comprimento_braco))

p = comprimento_braco
o = 1
for i in range(comprimento_prender_ima // (2*raio_ondulacao)):
    w = w.threePointArc((-largura_braco / 2 + raio_ondulacao*o + raio_arredondamentos, p - (raio_ondulacao+o*raio_arredondamentos)), (-largura_braco / 2, p - 2*(raio_ondulacao+o*raio_arredondamentos)))
    p -= 2*(raio_ondulacao+o*raio_arredondamentos)
    o = -1 * o

w = w.lineTo(-largura_braco / 2, largura_braco).lineTo(-raio_base_acoplamento_2, 0)
w = w.close().extrude(altura_braco)
#w = w.edges('#Z').fillet(raio_arredondamentos/2)
w = w.union(w.shell(raio_arredondamentos)).clean()#w.faces('|Z').edges().fillet(raio_arredondamentos)

w = w.union(acoplamento.full).clean()
w = w.cut(acoplamento.neg).clean()

#point = w.edges('>Z and %circle').vals()[0].arcCenter()

#w = w.edges(BoxSelector((point.x+profundidade_base_acoplamento,point.y+profundidade_base_acoplamento,point.z+profundidade_base_acoplamento), (point.x-profundidade_base_acoplamento,point.y-profundidade_base_acoplamento,point.z-profundidade_base_acoplamento)))
#w = w.fillet(raio_arredondamentos)

show_object(w)
if False:
    exporters.export(w, 'braco_2.stl')
if False:
    exporters.export(w_sup, 'suporte_ima.stl')