import cadquery as cq
from cq_gears import SpurGear, RingGear, PlanetaryGearset, HerringbonePlanetaryGearset
from cadquery import exporters

# em milímetros
comprimento_braco = 120
largura_braco = 16
altura_braco = 8

raio_bases = 14

dist_roda = 20
raio_roda = 12
altura_suporte_roda = 40 - raio_roda + altura_braco
largura_suporte_roda = 10

raio_arrendondamentos = 0.5

w = cq.Workplane('XY').box(comprimento_braco,
                           largura_braco,
                           altura_braco,
                           (False, True, False))

w = w.moveTo(0, 0).circle(raio_bases).extrude(altura_braco)

#w = w.edges('|X').fillet(1)
w = w.moveTo(comprimento_braco, 0).circle(raio_bases).extrude(altura_braco)

w = w.edges('|X').fillet(raio_arrendondamentos)

w = w.moveTo(comprimento_braco - dist_roda, 0).box(largura_suporte_roda, largura_braco, altura_suporte_roda, (True, True, False))

w = w.edges('|X').fillet(raio_arrendondamentos)
w = w.edges('|Z').fillet(raio_arrendondamentos)
w = w.edges('|Y').fillet(raio_arrendondamentos)

show_object(w)

# Create a gear object with the SpurGear class
#spur_gear = SpurGear(module=0.816, teeth_number=20, width=5.0, bore_d=2.5)

#right_spur_gear = SpurGear(module=0.8, teeth_number=20, width=5.0, bore_d=2.5)

# Build this gear using the gear function from cq.Workplane
# wp = cq.Workplane('XY').gear(spur_gear)

#straight_ring = RingGear(module=0.8, teeth_number=20, width=5.0, rim_width=2.5, clearance=0.1, backlash=-0.8)

#wp = cq.Workplane('XY').gear(straight_ring)

#wp2 = cq.Workplane('XY').gear(spur_gear)

#wp3 = wp.cut(wp2.findSolid())

#wp4 = cq.Workplane('XY').gear(right_spur_gear)

#exporters.export(wp3, 'teste_acoplamento.stl')

#wp5 = cq.Workplane('XY').circle(7).extrude(5)

#wp6 = cq.Workplane('XY').circle(8.8).extrude(5)

#gear = SpurGear(module=0.80, teeth_number=20, width=7.0, bore_d=2.5)

#wp7 = cq.Workplane('XY').gear(gear)


#gearset = PlanetaryGearset(module=1.0,
                           #sun_teeth_number=12, planet_teeth_number=18,
                           #width=10.0, rim_width=3.0, n_planets=3,
                           #bore_d=6.0)

#gearset = HerringbonePlanetaryGearset(module=1.0,
                                      #sun_teeth_number=18,
                                      #planet_teeth_number=18,
                                      #width=10.0, rim_width=3.0, n_planets=4,
                                      #helix_angle=30.0,
                            # Set backlash and clearance for 3d-prinatability
                                      #backlash=0.3, clearance=0.2, bore_d=6.0)

#wp = cq.Workplane('XY').gear(gearset)