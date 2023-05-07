import cadquery as cq
from cadquery import exporters
from motor_2 import motor_2

def acoplamento_motor_2(profundidade=7.2, altura_total=10, translate=False):
    w_motor = motor_2()
    full = cq.Workplane('XY', origin=(-w_motor.raio + 6.5, 0, w_motor.altura + 9.5 + altura_total - profundidade)).circle(4.5).extrude(-altura_total)
    w_ac = full.cut(w_motor).clean()
    neg = full.intersect(w_motor).clean()
    if translate:
        full = full.translate((w_motor.raio - 6.5, 0, -w_motor.altura - 9.5 + profundidade))
        w_ac = w_ac.translate((w_motor.raio - 6.5, 0, -w_motor.altura - 9.5 + profundidade))
        neg = neg.translate((w_motor.raio - 6.5, 0, -w_motor.altura - 9.5 + profundidade))
    w_ac.full = full
    w_ac.neg = neg
    return w_ac


#raise Exception(f"{__name__}")
if __name__ == "temp":
    w_motor = motor_2()
    #show_object(w_motor)
    w_eixo1 = cq.Workplane('XY', origin=(-w_motor.raio + 6.5, 0, w_motor.altura + 9.5)).circle(4.5).extrude(-6)
    w_eixo1 = w_motor.intersect(w_eixo1).clean()
    w_eixo2 = cq.Workplane('XY', origin=(-w_motor.raio + 6.5, 0, w_motor.altura + 3.5)).circle(4.5).extrude(-2)
    w_eixo2 = w_motor.intersect(w_eixo2).clean()
    w_eixo_00 = w_eixo1.union(w_eixo2)
    w_eixo_01 = w_eixo1.shell(0.1).union(w_eixo2.shell(0.1)).clean()
    w_eixo_02 = w_eixo1.shell(0.2).union(w_eixo2.shell(0.2)).clean()
    w_eixo_03 = w_eixo1.shell(0.3).union(w_eixo2.shell(0.3)).clean()
    w_eixo_04 = w_eixo1.shell(0.4).union(w_eixo2.shell(0.4)).clean()
    
    w_ac = acoplamento_motor_2()
    w_0_0mm = w_ac
    w_0_1mm = w_ac.cut(w_eixo_01).clean().translate((-7, 0, 0)).faces('>Z').fillet(0.5)
    w_0_2mm = w_ac.cut(w_eixo_02).clean().translate((-21, 0, 0)).faces('>Z').fillet(0.5)
    w_0_3mm = w_ac.cut(w_eixo_03).clean().translate((-35, 0, 0)).faces('>Z').fillet(0.5)
    w_0_4mm = w_ac.cut(w_eixo_04).clean().translate((-49, 0, 0)).faces('>Z').fillet(0.5)
    
    show_object(w_0_0mm)
    show_object(w_0_1mm)
    show_object(w_0_2mm)
    show_object(w_0_3mm)
    show_object(w_0_4mm)
    
    if False:
        exporters.export(w_0_0mm, 'novo_teste_0.0mm.stl')
        exporters.export(w_0_1mm, 'novo_teste_0.1mm.stl')
        exporters.export(w_0_2mm, 'novo_teste_0.2mm.stl')
        exporters.export(w_0_3mm, 'novo_teste_0.3mm.stl')
        exporters.export(w_0_4mm, 'novo_teste_0.4mm.stl')
    