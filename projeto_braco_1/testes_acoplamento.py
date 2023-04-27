import cadquery as cq
from cq_gears import SpurGear, RingGear, PlanetaryGearset, HerringbonePlanetaryGearset
from cadquery import exporters

SpurGear.ka = 1.25
RingGear.ka = 1.25

def acoplamento(profundidade=3.2, altura_total=4.8, raio_total=6.24, arredondamento=0.75):
    ang = 20.0

    # engrenagem levemente maior para cortar pedacinho do anel exterior
    spur_gear = SpurGear(module=0.828 / 2, teeth_number=20, width=profundidade, bore_d=2.5, pressure_angle=ang+4)
    
    # engrenagem que mais se aproxima da real
    right_spur_gear = SpurGear(module=0.8 / 2, teeth_number=20, width=profundidade, bore_d=2.5, pressure_angle=ang)
    
    # anel exterior para acoplar na engrenagem do motor
    straight_ring = RingGear(module=0.8 / 2, teeth_number=20, width=profundidade, rim_width=raio_total - 4.74, pressure_angle=ang, clearance=0.24, backlash=-1.8)
    
    wp = cq.Workplane('XY').gear(straight_ring)
    
    wp2 = cq.Workplane('XY').gear(spur_gear)
    
    wp3 = wp.cut(wp2.findSolid())
    wp3 = wp3.moveTo(0, 0).circle(straight_ring.rim_r).extrude(profundidade - altura_total)
    
    wp3 = wp3.faces('<Z').edges().fillet(arredondamento)
    wp3 = wp3.edges('%circle and >Z').fillet(arredondamento)
    wp3 = wp3.translate((0, 0, altura_total - profundidade))
    
    return (wp3, straight_ring.rim_r)
    #wp4 = cq.Workplane('XY').gear(right_spur_gear)
    
    #exporters.export(wp3, 'teste_acoplamento_v50.stl')
    
    #wp5 = cq.Workplane('XY').circle(7.0 / 2).extrude(altura)
    
    #wp6 = cq.Workplane('XY').circle(8.8 / 2).extrude(altura)
    
    
#raise Exception(f"{__name__}")
if __name__ == "temp":
    obj = acoplamento()
    show_object(obj[0])
    exporters.export(obj[0], 'teste_acoplamento_v50.stl')
    