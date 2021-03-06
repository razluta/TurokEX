//-----------------------------------------------------------------------------
//
// ComponentPlayerWeapon.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

const PWPN_STATE_READY          = 0;
const PWPN_STATE_SWAPOUT        = 1;
const PWPN_STATE_SWAPIN         = 2;
const PWPN_STATE_FIRING         = 3;
const PWPN_STATE_HOLDSTER       = 4;

ComponentPlayerWeapon = class.extendStatic(Component);

class.properties(ComponentPlayerWeapon,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    anim_Idle       : "",
    anim_Walk       : "",
    anim_Run        : "",
    anim_Fire       : "",
    anim_SwapIn     : "",
    anim_SwapOut    : "",
    x               : 0,
    y               : 0,
    z               : 0,
    state           : PWPN_STATE_READY,
    playSpeed       : 4.0,
    thudOffset      : 0.0,
    bOwned          : false,
    readySound      : "",
    translation     : null,
    actorType       : "",
    bActive         : false,
    bHasAltAmmo     : false,
    bAltAmmoSet     : false,
    playerOwner     : null,
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    spawn : function(playerComponent)
    {
        this.playerOwner = playerComponent;
        
        var actor = Level.spawnActor(this.actorType, this.x, this.y, this.z,
            Math.PI, 0, null);
        
        actor.bHidden = true;
        actor.physics = 0;
        return actor;
    },
    
    initAnimations : function()
    {
    },
    
    switchAmmoType : function()
    {
        if(!this.bHasAltAmmo)
            return;
            
        this.bAltAmmoSet ^= 1;
    },
    
    checkHoldster : function()
    {
        var actor = this.parent.owner;
        
        if(ClientPlayer.component.controller.state == STATE_MOVE_CLIMB &&
            this.state != PWPN_STATE_HOLDSTER)
        {
            actor.blendAnim(this.anim_SwapOut,
                this.playSpeed, 4.0, 0);

            this.state = PWPN_STATE_HOLDSTER;

            return true;
        }

        return false;
    },
    
    change : function()
    {
        var actor = this.parent.owner;
        
        actor.blendAnim(this.anim_SwapOut,
            this.playSpeed, 4.0, 0);

        this.state = PWPN_STATE_SWAPOUT;
    },
    
    onBeginAttack : function()
    {
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
            
        return true;
    },
    
    onAttack : function()
    {
        if(this.parent.owner.animState.flags & NRender.ANIM_STOPPED)
        {
            this.readyAnim();
            this.state = PWPN_STATE_READY;
        }
    },
    
    checkAttack : function()
    {
        var actor = this.parent.owner;
        
        if(ClientPlayer.command.getAction('+attack'))
        {
            if(this.onBeginAttack())
            {
                this.state = PWPN_STATE_FIRING;
                return true;
            }
        }
        
        return false;
    },
    
    checkWeaponChange : function()
    {
        if(ClientPlayer.command.getAction('+nextweap') &&
            ClientPlayer.command.getActionHeldTime('+nextweap') == 0)
        {
            ClientPlayer.component.cycleNextWeapon();
            return true;
        }
        
        if(ClientPlayer.command.getAction('+prevweap') &&
            ClientPlayer.command.getActionHeldTime('+prevweap') == 0)
        {
            ClientPlayer.component.cyclePrevWeapon();
            return true;
        }
        
        return false;
    },
    
    readyAnim : function()
    {
        var actor = this.parent.owner;
        
        if(actor.animState.flags & NRender.ANIM_BLEND)
            return;
            
        if(Sys.getCvar('g_weaponbobbing') > 0)
        {
            actor.blendAnim(this.anim_Idle,
                this.playSpeed, 8.0, NRender.ANIM_LOOP);
            return;
        }
        
        var d = ClientPlayer.worldState.accel.unit2();
        
        if(d >= 1.35)
        {
            actor.blendAnim(this.anim_Run,
                this.playSpeed, 8.0, NRender.ANIM_LOOP);
        }
        else if(d >= 0.1)
        {
            actor.blendAnim(this.anim_Walk,
                this.playSpeed, 8.0, NRender.ANIM_LOOP);
        }
        else
        {
            actor.blendAnim(this.anim_Idle,
                this.playSpeed, 8.0, NRender.ANIM_LOOP);
        }
    },
    
    spawnFx : function(fx, x, y, z, bMuzzle)
    {
        var actor = ClientPlayer.actor;
        var org = ClientPlayer.component.controller.origin;
        var vec = Vector.applyRotation(new Vector(x, y, z), actor.rotation);
        
        vec.x += org.x;
        vec.y += org.y + 56.32;
        vec.z += org.z;

        Sys.spawnFx(fx, actor, vec, actor.rotation,
            Plane.fromIndex(actor.plane), null, null, bMuzzle);
    },
    
    //------------------------------------------------------------------------
    // STATES
    //------------------------------------------------------------------------
    
    ready : function()
    {
        if(this.checkHoldster())
            return;
            
        if(this.checkAttack())
            return;
            
        if(this.checkWeaponChange())
            return;
            
        this.readyAnim();
    },
    
    fire : function()
    {
        if(this.checkHoldster())
        {
            this.parent.owner.animState.flags &= ~NRender.ANIM_LOOP;
            return;
        }
        
        this.onAttack();
    },
    
    holdster : function()
    {
        if(ClientPlayer.component.controller.state != STATE_MOVE_CLIMB)
        {
            this.parent.owner.setAnim(this.anim_SwapIn,
                this.playSpeed, 0);

            this.state = PWPN_STATE_SWAPIN;
        }
    },
    
    swapOut : function()
    {
        if(this.checkHoldster())
            return;
            
        if(this.checkWeaponChange())
            return;
            
        if(this.parent.owner.animState.flags & NRender.ANIM_STOPPED)
        {
            ClientPlayer.component.setNewWeapon();
            
            var newWpn = ClientPlayer.component.activeWeapon;
            this.parent.owner.bHidden = true;
            
            Snd.play(newWpn.readySound);
            newWpn.parent.owner.setAnim(newWpn.anim_SwapIn,
                newWpn.playSpeed, 0);

            newWpn.state = PWPN_STATE_SWAPIN;
        }
    },
    
    swapIn : function()
    {
        if(this.checkHoldster())
            return;
            
        if(this.checkWeaponChange())
            return;
            
        if(this.parent.owner.animState.flags & NRender.ANIM_STOPPED)
        {
            this.readyAnim();
            this.state = PWPN_STATE_READY;
        }
    },
    
    //------------------------------------------------------------------------
    // EVENTS
    //------------------------------------------------------------------------
    
    start : function()
    {
    },
    
    onReady : function()
    {
        this.translation = new Vector();
        this.initAnimations();
        this.parent.owner.owner = ClientPlayer.actor;
    },
    
    tick : function()
    {
        const WEAPONTURN_MAX        = 0.08;
        const WEAPONTURN_EPSILON    = 0.001;
        
        var actor = this.parent.owner;
    
        actor.yaw -= (ClientPlayer.command.mouse_x * 0.00175);
        var an = Angle.invertClampSum(actor.yaw, Math.PI);

        if(an >  WEAPONTURN_MAX) actor.yaw =  (Math.PI - WEAPONTURN_MAX);
        if(an < -WEAPONTURN_MAX) actor.yaw = -(Math.PI - WEAPONTURN_MAX);
        
        if(actor.yaw < 0)
            actor.yaw = Math.lerp(actor.yaw, -Math.PI, 0.1);
        else
            actor.yaw = Math.lerp(actor.yaw, Math.PI, 0.1);
            
        if(actor.yaw > (Math.PI - WEAPONTURN_EPSILON) &&
            actor.yaw < -(Math.PI - WEAPONTURN_EPSILON))
        {
            actor.yaw = Math.PI;
        }
        
        actor.pitch = (actor.pitch -
            (ClientPlayer.command.mouse_y * 0.00175)) * 0.9;
            
        if(actor.pitch >  WEAPONTURN_MAX) actor.pitch =  WEAPONTURN_MAX;
        if(actor.pitch < -WEAPONTURN_MAX) actor.pitch = -WEAPONTURN_MAX;
        if(actor.pitch <  WEAPONTURN_EPSILON &&
            actor.pitch > -WEAPONTURN_EPSILON)
        {
            actor.pitch = 0;
        }
        
        var wstate = ClientPlayer.worldState;
        var controller = ClientPlayer.component.controller;
        
        // lean weapon if strafing
        if(controller != null && controller.state != STATE_MOVE_SWIM)
            actor.roll = wstate.roll * 0.75;
        
        if(wstate.plane != null)
        {
            this.translation.copy(actor.origin);
            
            // add a little vertical force to weapon if jumping or falling
            offset = (wstate.origin.y - wstate.plane.distance(wstate.origin));
            
            var velocity = wstate.velocity.y * wstate.frameTime;

            if(!(offset < 0.2) && (velocity < 0.2 || velocity > 0.2))
            {
                offset = velocity;
                if(velocity > 0)
                {
                    // cut back offset a little if jumping
                    offset *= 0.35;
                }
            }
            else
            {
                offset = 0;
            }
            
            var bob_xz = 0;
            var bob_y = 0;
            
            if(Sys.getCvar('g_weaponbobbing') > 0)
            {
                var pa = ClientPlayer.actor;
            
                if(controller.state == STATE_MOVE_WALK &&
                    (wstate.origin.y + wstate.velocity.y) -
                    wstate.plane.distance(wstate.origin) < pa.height)
                {
                    const WEPBOB_EPISILON  = 0.001;
                    const WEPBOB_MAXSWAY   = Angle.degToRad(22.5);
                    const WEPBOB_FREQ      = 0.007;
                    const WEPBOB_FREQY     = 0.014;
                    const WEPBOB_ANGLE     = 8;
                    
                    var d = Math.abs(wstate.accel.z * wstate.frameTime) * 0.06;
                    
                    if(d > WEPBOB_EPISILON)
                    {
                        if(d > WEPBOB_MAXSWAY)
                            d = WEPBOB_MAXSWAY;
                        
                        bob_xz = Math.sin(Sys.time() * WEPBOB_FREQ) * WEPBOB_ANGLE * d;
                        bob_y = Math.sin(Sys.time() * WEPBOB_FREQY) * WEPBOB_ANGLE * d;
                    }
                }
            }
            
            this.translation.x = this.x + bob_xz;
            this.translation.y = Math.lerp(this.translation.y, this.y + offset + bob_y, 0.25);
            this.translation.z = this.z + bob_xz;
            
            actor.origin = this.translation;
        }
        
        if(this.bActive == true)
        {
            switch(this.state)
            {
                case PWPN_STATE_READY:
                    this.ready();
                    break;
                    
                case PWPN_STATE_SWAPOUT:
                    this.swapOut();
                    break;
                    
                case PWPN_STATE_SWAPIN:
                    this.swapIn();
                    break;
                    
                case PWPN_STATE_FIRING:
                    this.fire();
                    break;
                    
                case PWPN_STATE_HOLDSTER:
                    this.holdster();
                    break;
                    
                default:
                    break;
            }
        }
        
        //actor.animState.update();
    }
});

//-----------------------------------------------------------------------------
//
// WeaponKnife.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponKnife = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponKnife,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x               : 170.667,
    y               : -9.21548,
    z               : -153.6003,
    actorType       : 'pweapon_knife',
    bCanAttack      : false,
    swishType       : 0,
    
    // new animations
    anim_FireAlt1   : null,
    anim_FireAlt2   : null,
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_FireAlt1  = "anim04";
        this.anim_FireAlt2  = "anim05";
        this.anim_SwapIn    = "anim06";
        this.anim_SwapOut   = "anim07";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        var fireAnim;
        var r = Sys.rand(100);
        
        if(r <= 32)
        {
            this.swishType = 0;
            fireAnim = this.anim_FireAlt1;
            Snd.play('sounds/shaders/knife_swish_1.ksnd');
        }
        else if(r <= 65)
        {
            this.swishType = 1;
            fireAnim = this.anim_Fire;
            Snd.play('sounds/shaders/knife_swish_2.ksnd');
        }
        else
        {
            this.swishType = 2;
            fireAnim = this.anim_FireAlt2;
            Snd.play('sounds/shaders/knife_swish_3.ksnd');
        }
        
        this.bCanAttack = true;
        this.parent.owner.blendAnim(fireAnim,
            this.playSpeed, 4.0, 0);
        return true;
    },
    
    onAttack : function()
    {
        var actor = this.parent.owner;
        
        if(actor.animState.playTime >= 0.15 && this.bCanAttack)
        {
            this.bCanAttack = false;
            ClientPlayer.component.aKnifeAttack();
        }
        
        ComponentPlayerWeapon.prototype.onAttack.bind(this)();
    }
});

//-----------------------------------------------------------------------------
//
// WeaponBow.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponBow = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponBow,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 133.12026,
    y           : -12.62882,
    z           : -150.18696,
    actorType   : 'pweapon_bow',
    bHasAltAmmo : true,
    bAiming     : false,
    
    // new animations
    anim_Aim    : null,
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Aim       = "anim03";
        this.anim_Fire      = "anim04";
        this.anim_SwapIn    = "anim05";
        this.anim_SwapOut   = "anim06";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        if(this.bAiming == true)
            return true;
        
        this.bAiming = true;
        Snd.play('sounds/shaders/bow_stretch.ksnd');
        this.parent.owner.blendAnim(this.anim_Aim,
            this.playSpeed, 18.0, NRender.ANIM_LOOP);
            
        return true;
    },
    
    onAttack : function()
    {
        if(ClientPlayer.command.getActionHeldTime('+attack') > 0 && this.bAiming)
            return;
        else
        {
            if(this.bAiming == true)
            {
                var time = this.parent.owner.animState.playTime;
                
                if(time > 0.4)
                    time = 0.4;
                
                Snd.play('sounds/shaders/bow_twang.ksnd');
                this.bAiming = false;
                this.parent.owner.blendAnim(this.anim_Fire,
                    this.playSpeed, 4.0, 0);
                
                var actor = this.playerOwner.parent.owner;
                var vec = actor.getLocalVector(0.2048, -6.144, 25.6);
                
                vec.y += actor.height;
                
                var dir = new Vector(0, 0, 1);
                dir.scale(time * 512.0 * 15.0);

                if(!this.bAltAmmoSet)
                {
                    this.playerOwner.ammo[AM_ARROWS].use(1);
                    Sys.spawnFx('fx/projectile_arrow.kfx', actor, vec, actor.rotation,
                        Plane.fromIndex(actor.plane),
                        Vector.applyRotation(dir, actor.rotation));
                }
                else
                {
                    this.playerOwner.ammo[AM_TEKARROWS].use(1);
                    Sys.spawnFx('fx/projectile_tekarrow.kfx', actor, vec, actor.rotation,
                        Plane.fromIndex(actor.plane),
                        Vector.applyRotation(dir, actor.rotation));
                }
            }
        }
        
        ComponentPlayerWeapon.prototype.onAttack.bind(this)();
    }
});

//-----------------------------------------------------------------------------
//
// WeaponPistol.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponPistol = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponPistol,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 160.42698,
    y           : -16,
    z           : -200.32036,
    readySound  : 'sounds/shaders/ready_pistol.ksnd',
    actorType   : 'pweapon_pistol',
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        this.spawnFx('fx/muzzle_pistol.kfx', -6.656, -3.2, 15.696, false);
        this.spawnFx('fx/bulletshell.kfx', -10.24, -10.24, 15.648, false);
        
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
        
        this.playerOwner.ammo[AM_CLIPS].use(1);
        ClientPlayer.component.aPistolAttack(); 
        return true;
    }
});

//-----------------------------------------------------------------------------
//
// WeaponShotgun.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponShotgun = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponShotgun,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 170.667,
    y           : -9.21548,
    z           : -170.667,
    readySound  : 'sounds/shaders/ready_shotgun.ksnd',
    actorType   : 'pweapon_shotgun',
    bReload     : false,
    bHasAltAmmo : true,
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        this.spawnFx('fx/muzzle_shotgun.kfx', -5.75, -3.5, 15.696);
        
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
        
        if(!this.bAltAmmoSet)
        {
            //this.playerOwner.ammo[AM_SHELLS].use(1);
            //this.spawnFx('fx/projectile_shell.kfx', -12.288, -7.1680, 25.6);
            ClientPlayer.component.aShotgunAttack();
        }
        else
        {
            Snd.play('sounds/shaders/riot_shotgun_shot.ksnd', this.playerOwner.parent.owner);
            this.playerOwner.recoilPitch = -0.0408;
            this.playerOwner.ammo[AM_EXPSHELLS].use(1);
            this.spawnFx('fx/fx_195.kfx', -12.288, -7.1680, 25.6);
        }
        return true;
    },
    
    onAttack : function()
    {
        if(this.parent.owner.animState.playTime >= 0.5)
        {
            if(!this.bReload)
            {
                this.bReload = true;
                
                if(!this.bAltAmmoSet)
                    this.spawnFx('fx/shotshell.kfx', -11.26, -4.0, 15.696);
                else
                    this.spawnFx('fx/shotshellexp.kfx', -11.26, -4.0, 15.696);
                    
                Snd.play('sounds/shaders/ready_shotgun.ksnd');
            }
        }
        else
            this.bReload = false;
            
        ComponentPlayerWeapon.prototype.onAttack.bind(this)();
    }
});

//-----------------------------------------------------------------------------
//
// WeaponAutoShotgun.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponAutoShotgun = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponAutoShotgun,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x               : 170.667,
    y               : -2.3888,
    z               : -177.49368,
    actorType       : 'pweapon_autoshotgun',
    bReload         : false,
    spinAngle       : 0.0,
    spinRotation    : new Quaternion(0, 0, 0, 1),
    bHasAltAmmo     : true,
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    tick : function()
    {
        if(this.spinAngle > 0)
            this.spinAngle -= 0.03;
        else
            this.spinAngle = 0;
        
        this.spinRotation.setRotation(this.spinAngle, 1, 0, 0);
        this.parent.owner.setNodeRotation(1, this.spinRotation);

        ComponentPlayerWeapon.prototype.tick.bind(this)();
    },
    
    onBeginAttack : function()
    {
        this.spawnFx('fx/muzzle_shotgun.kfx', -6.144, -1.9456, 15.696);
        if(!this.bAltAmmoSet)
            this.spawnFx('fx/shotshell.kfx', -14.336, -12.288, 20.736);
        else
            this.spawnFx('fx/shotshellexp.kfx', -14.336, -12.288, 20.736);
        
        if(!this.bAltAmmoSet)
        {
            this.playerOwner.ammo[AM_SHELLS].use(1);
            ClientPlayer.component.aAutoShotgunAttack();
        }
        else
        {
            Snd.play('sounds/shaders/riot_shotgun_shot.ksnd', this.playerOwner.parent.owner);
            this.playerOwner.recoilPitch = -0.0408;
            this.playerOwner.ammo[AM_EXPSHELLS].use(1);
            this.spawnFx('fx/fx_195.kfx', -12.288, -7.1680, 25.6);
        }
        
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
        
        this.bReload = false;
        this.spinAngle = Angle.degToRad(36);
        return true;
    },
    
    onAttack : function()
    {
        if(this.parent.owner.animState.playTime >= 0.0125)
        {
            if(!this.bReload)
            {
                this.bReload = true;
                Snd.play('sounds/shaders/reload_auto_shotgun.ksnd');
            }
        }
        else
            this.bReload = false;
            
        ComponentPlayerWeapon.prototype.onAttack.bind(this)();
    }
});

//-----------------------------------------------------------------------------
//
// WeaponRifle.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponRifle = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponRifle,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 170.667,
    y           : -19.4555,
    z           : -204.8004,
    actorType   : 'pweapon_rifle',
    bShotsFired : [false, false, false],
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onAttack : function()
    {
        var actor = this.parent.owner;
        
        if(actor.animState.playTime >= 0.0)
        {
            if(!this.bShotsFired[0])
            {
                this.bShotsFired[0] = true;
                this.spawnFx('fx/muzzle_rifle.kfx', -5.12, -3.584, 15.696);
                this.spawnFx('fx/bulletshell.kfx', -10.24, -10.24, 13.82);
                this.playerOwner.ammo[AM_CLIPS].use(1);
                ClientPlayer.component.aRifleAttack();
            }
        }
        
        if(actor.animState.playTime >= 0.1)
        {
            if(!this.bShotsFired[1])
            {
                this.bShotsFired[1] = true;
                this.spawnFx('fx/muzzle_rifle.kfx', -5.12, -3.584, 15.696);
                this.spawnFx('fx/bulletshell.kfx', -10.24, -10.24, 13.82);
                this.playerOwner.ammo[AM_CLIPS].use(1);
                ClientPlayer.component.aRifleAttack();
            }
        }
        
        if(actor.animState.playTime >= 0.2)
        {
            if(!this.bShotsFired[2])
            {
                this.bShotsFired[2] = true;
                this.spawnFx('fx/muzzle_rifle.kfx', -5.12, -3.584, 15.696);
                this.spawnFx('fx/bulletshell.kfx', -10.24, -10.24, 13.82);
                this.playerOwner.ammo[AM_CLIPS].use(1);
                ClientPlayer.component.aRifleAttack();
            }
        }
        
        if(actor.animState.flags & NRender.ANIM_STOPPED)
        {
            this.readyAnim();
            this.state = PWPN_STATE_READY;
            
            this.bShotsFired[0] = false;
            this.bShotsFired[1] = false;
            this.bShotsFired[2] = false;
        }
    }
});

//-----------------------------------------------------------------------------
//
// WeaponPulseRifle.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponPulseRifle = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponPulseRifle,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 170.667,
    y           : -9.21548,
    z           : -170.667,
    actorType   : 'pweapon_pulserifle',
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim05";
        this.anim_SwapIn    = "anim03";
        this.anim_SwapOut   = "anim04";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, NRender.ANIM_LOOP);
        
        this.parent.owner.animState.playTime = 0.08;
        return true;
    },
    
    onAttack : function()
    {
        var actor = this.parent.owner;
        
        if(actor.animState.playTime >= 0.16)
        {
            actor.animState.playTime -= 0.16;
            this.playerOwner.ammo[AM_CELLS].use(1);
            var cActor = ClientPlayer.actor;
            var org = this.playerOwner.controller.origin;
            var vec = Vector.applyRotation(new Vector(-5.12, -14.336, -107.52), cActor.rotation);
            
            vec.x += org.x;
            vec.y += org.y + 56.32;
            vec.z += org.z;

            Sys.spawnFx('fx/projectile_pulseshot.kfx', cActor, vec, cActor.rotation,
                Plane.fromIndex(cActor.plane));
            
            this.playerOwner.recoilPitch = -0.0288;
            Snd.play('sounds/shaders/machine_gun_shot_2.ksnd',
                this.playerOwner.parent.owner);
        }
        
        if(!ClientPlayer.command.getAction('+attack'))
        {
            this.readyAnim();
            this.state = PWPN_STATE_READY;
        }
    }
});

//-----------------------------------------------------------------------------
//
// WeaponGrenadeLauncher.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponGrenadeLauncher = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponGrenadeLauncher,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 170.667,
    y           : -9.21548,
    z           : -153.6003,
    readySound  : 'sounds/shaders/ready_grenade_launcher.ksnd',
    actorType   : 'pweapon_grenadelauncher',
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
     
        this.playerOwner.ammo[AM_GRENADES].use(1);
        Snd.play('sounds/shaders/grenade_launch.ksnd', ClientPlayer.actor);
        this.spawnFx('fx/projectile_grenade.kfx', -18.432, -5.12, -15.696);
        this.spawnFx('fx/muzzle_grenade_launcher.kfx', -10.35, -2.048, 18.432);
        return true;
    }
});

//-----------------------------------------------------------------------------
//
// WeaponMiniGun.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponMiniGun = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponMiniGun,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x               : 163.84032,
    y               : -5.80214,
    z               : -163.84032,
    actorType       : 'pweapon_minigun',
    spinSpeed       : 0.0,
    spinAngle       : 0.0,
    spinRotation    : new Quaternion(0, 0, 0, 1),
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim05";
        this.anim_SwapIn    = "anim03";
        this.anim_SwapOut   = "anim04";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    tick : function()
    {
        if(this.state == PWPN_STATE_FIRING)
            this.spinSpeed = Math.lerp(this.spinSpeed, 40.0, 0.35);
        else
            this.spinSpeed = Math.lerp(this.spinSpeed, 0.0, 0.025);
            
        this.spinAngle += this.spinSpeed * Sys.deltatime();
        this.spinRotation.setRotation(this.spinAngle, 0, 1, 0);
        this.parent.owner.setNodeRotation(1, this.spinRotation);

        ComponentPlayerWeapon.prototype.tick.bind(this)();
    },
    
    onBeginAttack : function()
    {
        Snd.play('sounds/shaders/mini_gun_whir.ksnd', ClientPlayer.actor);
        
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, NRender.ANIM_LOOP);
        
        return true;
    },
    
    onAttack : function()
    {
        var actor = this.parent.owner;
        
        if(actor.animState.playTime >= 0.11)
        {
            actor.animState.playTime -= 0.11;
            // TODO
            this.spawnFx('fx/fx_037.kfx', -4.608, -3.1744, 14.848);
            this.spawnFx('fx/bulletshell.kfx', -10.24, -10.24, 13.82);
            Snd.play('sounds/shaders/mini_gun_shot.ksnd');
            this.playerOwner.aMinigunAttack();
            
            //var pActor = this.playerOwner.parent.owner;
            //var vec = pActor.getLocalVector(8.192, -10.24, 25.6);
            //vec.y += (pActor.centerHeight + pActor.viewHeight) * 0.72;

            //Sys.spawnFx('fx/projectile_minibullet.kfx', pActor, vec, pActor.rotation,
                //Plane.fromIndex(pActor.plane));
        }
        
        if(!ClientPlayer.command.getAction('+attack'))
        {
            Snd.play('sounds/shaders/minigun_stop.ksnd');
            this.readyAnim();
            this.state = PWPN_STATE_READY;
        }
    }
});

//-----------------------------------------------------------------------------
//
// WeaponAlienRifle.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponAlienRifle = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponAlienRifle,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 170.667,
    y           : -9.21548,
    z           : -153.6003,
    readySound  : 'sounds/shaders/ready_tek_weapon_1.ksnd',
    actorType   : 'pweapon_alienrifle',
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        this.spawnFx('fx/projectile_tekshot.kfx', -4.096, -14.336, -25.6);
        Snd.play('sounds/shaders/tek_weapon_1.ksnd');
        
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
            
        return true;
    }
});

//-----------------------------------------------------------------------------
//
// WeaponRocketLauncher.js
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

WeaponRocketLauncher = class.extendStatic(ComponentPlayerWeapon);

class.properties(WeaponRocketLauncher,
{
    //------------------------------------------------------------------------
    // VARS
    //------------------------------------------------------------------------
    
    x           : 170.667,
    y           : -9.21548,
    z           : -153.6003,
    readySound  : 'sounds/shaders/ready_missile_launcher.ksnd',
    actorType   : 'pweapon_rocketlauncher',
    
    //------------------------------------------------------------------------
    // FUNCTIONS
    //------------------------------------------------------------------------
    
    initAnimations : function()
    {
        var actor = this.parent.owner;
        
        this.anim_Idle      = "anim00";
        this.anim_Walk      = "anim01";
        this.anim_Run       = "anim02";
        this.anim_Fire      = "anim03";
        this.anim_SwapIn    = "anim04";
        this.anim_SwapOut   = "anim05";
        
        actor.setAnim(this.anim_Idle, 4.0, NRender.ANIM_LOOP);
    },
    
    onBeginAttack : function()
    {
        this.spawnFx('fx/projectile_rocket.kfx', -56.32, -14.336, -25.6);
        Snd.play('sounds/shaders/missile_launch.ksnd');
        Snd.play('sounds/shaders/reload_missile_launcher.ksnd');
        
        this.parent.owner.blendAnim(this.anim_Fire,
            this.playSpeed, 4.0, 0);
            
        return true;
    }
});
