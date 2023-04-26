import cadquery as cq
from cq_gears import SpurGear, RingGear, PlanetaryGearset, HerringbonePlanetaryGearset
from cadquery import exporters

SpurGear.ka = 1.25
RingGear.ka = 1.25

ang = 20.0
altura = 3.2

# Create a gear object with the SpurGear class
spur_gear = SpurGear(module=0.828 / 2, teeth_number=20, width=altura, bore_d=2.5, pressure_angle=ang+4)

right_spur_gear = SpurGear(module=0.8 / 2, teeth_number=20, width=altura, bore_d=2.5, pressure_angle=ang)

# Build this gear using the gear function from cq.Workplane
# wp = cq.Workplane('XY').gear(spur_gear)

straight_ring = RingGear(module=0.8 / 2, teeth_number=20, width=altura, rim_width=1.5, pressure_angle=ang, clearance=0.24, backlash=-1.8)

wp = cq.Workplane('XY').gear(straight_ring)

wp2 = cq.Workplane('XY').gear(spur_gear)

wp3 = wp.cut(wp2.findSolid())
wp3 = wp3.moveTo(0, 0).circle(straight_ring.rim_r).extrude(-1.6)


#wp4 = cq.Workplane('XY').gear(right_spur_gear)

exporters.export(wp3, 'teste_acoplamento_v50.stl')

wp5 = cq.Workplane('XY').circle(7.0 / 2).extrude(altura)

wp6 = cq.Workplane('XY').circle(8.8 / 2).extrude(altura)

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