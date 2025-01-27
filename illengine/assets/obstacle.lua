local entity = ...
p = GetPosition(entity)
v = GetVelocity(entity)
g = GetGravity(entity)
c = GetCollider(entity)
player = GetPlayer()
ppos = GetPosition(player)
hp = GetHealth(player)
r = GetHitRate(player)
if(ppos.y < p.y and ppos.x >= p.x - 10 and ppos.x <= p.x + 10) then
    g = -0.5
end
lives = GetLives()
if (Collides(entity, player) and r > 30) then
    hp = hp - 1
    r = 0
    SetHitRate(player, r)
    Destroy(lives[1])
    if(hp <= 0) then GameOver() end
end
v.y = v.y + g
p.y = p.y + v.y
SetHealth(player, hp)
SetGravity(entity, g)