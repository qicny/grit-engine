#include <OgreViewport.h>
#include <OgreRenderTarget.h>

#include "Grit.h"

#include "lua_wrappers_render.h"
#include "lua_wrappers_tex.h"
#include "lua_wrappers_mobj.h"

#include "lua_userdata_dependency_tracker_funcs.h"

// VIEWPORT ================================================================ {{{


void push_viewport (lua_State *L, Ogre::Viewport *vp)
{
        if (!vp) {
                lua_pushnil(L);
                return;
        }
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (vp);
        luaL_getmetatable(L, VIEWPORT_TAG);
        lua_setmetatable(L, -2);
        Ogre::RenderTarget *rt = vp->getTarget();
        render_target_maps& maps = grit->getUserDataTables().rts[rt];
        maps.viewports[vp].push_back(ud);
}

static int viewport_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        Ogre::RenderTarget *rt = self.getTarget();
        rt->removeViewport(self.getZOrder());
        map_nullify_remove(grit->getUserDataTables().rts[rt].viewports,&self);
        return 0;
TRY_END
}

static int viewport_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::Viewport,self,1,VIEWPORT_TAG,0);
        if (self==NULL) return 0;
        Ogre::RenderTarget *rt = self->getTarget();
        vec_nullify_remove(grit->getUserDataTables().rts[rt].viewports[self],
                           &self);
        return 0;
TRY_END
}



static int viewport_set_background_colour (lua_State *L)
{
TRY_START
        check_args(L,4);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        Ogre::Real r = luaL_checknumber(L,2);
        Ogre::Real g = luaL_checknumber(L,3);
        Ogre::Real b = luaL_checknumber(L,4);
        self.setBackgroundColour(Ogre::ColourValue(r,g,b));
        return 0;
TRY_END
}


static int viewport_add_compositor (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        std::string name = luaL_checkstring(L,2);
        Ogre::CompositorManager::getSingleton().addCompositor(&self,name);
        return 0;
TRY_END
}

static int viewport_remove_compositor (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        std::string name = luaL_checkstring(L,2);
        Ogre::CompositorManager::getSingleton().removeCompositor(&self,name);
        return 0;
TRY_END
}

static int viewport_set_compositor_enabled (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        std::string name = luaL_checkstring(L,2);
        bool v = 0!=lua_toboolean(L,3);
        Ogre::CompositorManager::getSingleton()
                .setCompositorEnabled(&self,name,v);
        return 0;
TRY_END
}

TOSTRING_ADDR_MACRO(viewport,Ogre::Viewport,VIEWPORT_TAG)

static int viewport_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key=="setBackgroundColour") {
                push_cfunction(L,viewport_set_background_colour);
        } else if (key=="addCompositor") {
                push_cfunction(L,viewport_add_compositor);
        } else if (key=="removeCompositor") {
                push_cfunction(L,viewport_remove_compositor);
        } else if (key=="setCompositorEnabled") {
                push_cfunction(L,viewport_set_compositor_enabled);
        } else if (key=="destroy") {
                push_cfunction(L,viewport_destroy);
        } else if (key=="actualLeft") {
                lua_pushnumber(L,self.getActualLeft());
        } else if (key=="actualTop") {
                lua_pushnumber(L,self.getActualTop());
        } else if (key=="actualWidth") {
                lua_pushnumber(L,self.getActualWidth());
        } else if (key=="actualHeight") {
                lua_pushnumber(L,self.getActualHeight());
        } else if (key=="left") {
                lua_pushnumber(L,self.getLeft());
        } else if (key=="top") {
                lua_pushnumber(L,self.getTop());
        } else if (key=="width") {
                lua_pushnumber(L,self.getWidth());
        } else if (key=="height") {
                lua_pushnumber(L,self.getHeight());
        } else if (key=="zOrder") {
                lua_pushnumber(L,self.getZOrder());
        } else if (key=="clearEveryFrame") {
                lua_pushboolean(L,self.getClearEveryFrame());
        } else if (key=="overlaysEnabled") {
                lua_pushboolean(L,self.getOverlaysEnabled());
        } else {
                my_lua_error(L,"Not a valid Viewport member: "+key);
        }
        return 1;
TRY_END
}

static int viewport_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Viewport,self,1,VIEWPORT_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key=="zOrder") {
                my_lua_error(L,"Can only set Z order when viewport created.");
        } else if (key=="clearEveryFrame") {
                bool v = 0!=lua_toboolean(L,3);
                self.setClearEveryFrame(v);
        } else if (key=="overlaysEnabled") {
                bool v = 0!=lua_toboolean(L,3);
                self.setOverlaysEnabled(v);
        } else {
                my_lua_error(L,"Not a valid Viewport member: "+key);
        }
        return 1;
TRY_END
}

EQ_PTR_MACRO(Ogre::Viewport,viewport,VIEWPORT_TAG)

MT_MACRO_NEWINDEX(viewport);

//}}}


// RENDER TARGET =========================================================== {{{

static Ogre::RenderTarget *check_rt(lua_State *L, int index)
{
        Ogre::RenderTarget *ptr = NULL;
        if (has_tag(L,index,RWIN_TAG)) {
                ptr = *static_cast<Ogre::RenderWindow**>
                        (lua_touserdata(L,index));
        } else if (has_tag(L,index,RTEX_TAG)) {
                ptr = *static_cast<Ogre::RenderTexture**>
                        (lua_touserdata(L,index));
        }

        if (!ptr) {
                std::string acceptable_types;
                acceptable_types += RWIN_TAG ", or ";
                acceptable_types += RTEX_TAG;
                luaL_typerror(L,index,acceptable_types.c_str());
        }
        return ptr;
}

static int rt_destroy(lua_State *L)
{
TRY_START
        check_args(L,1);
        Ogre::RenderTarget *rt = check_rt(L, 1);
        grit->getOgre()->getRenderSystem()->destroyRenderTarget(rt->getName());
        struct render_target_maps& maps = grit->getUserDataTables().rts[rt];
        vec_nullify_remove_all(maps.userData);
        map_nullify_remove_all(maps.viewports);
        map_remove_only(grit->getUserDataTables().rts,rt);
        return 0;
TRY_END
}

static int rt_add_viewport (lua_State *L)
{
TRY_START
        check_args(L,7);
        Ogre::RenderTarget& self = *check_rt(L,1);
        GET_UD_MACRO(Ogre::Camera,cam,2,CAM_TAG);
        int zorder = check_t<int>(L,3);
        lua_Number left = luaL_checknumber(L,4);
        lua_Number top = luaL_checknumber(L,5);
        lua_Number width = luaL_checknumber(L,6);
        lua_Number height = luaL_checknumber(L,7);

        push_viewport(L,self.addViewport (&cam,zorder,left,top,width,height));
        return 1;
TRY_END
}


static int rt_update (lua_State *L)
{
TRY_START
        check_args(L,1);
        Ogre::RenderTarget& self = *check_rt(L,1);
        self.update();
        return 0;
TRY_END
}

static int rt_write_contents_to_file (lua_State *L)
{
TRY_START
        check_args(L,2);
        Ogre::RenderTarget& self = *check_rt(L,1);
        std::string filename = luaL_checkstring(L,2);
        self.writeContentsToFile(filename);
        return 0;
TRY_END
}

static bool rt_index(lua_State *L, Ogre::RenderTarget *rt,
                     const std::string& key)
{
TRY_START
        if (key=="colourDepth") {
                lua_pushnumber(L,rt->getColourDepth());
        } else if (key=="width") {
                lua_pushnumber(L,rt->getWidth());
        } else if (key=="height") {
                lua_pushnumber(L,rt->getHeight());
        } else if (key=="name") {
                lua_pushstring(L,rt->getName().c_str());
        } else if (key=="destroy") {
                push_cfunction(L,rt_destroy);
        } else if (key=="update") {
                push_cfunction(L,rt_update);
        } else if (key=="addViewport") {
                push_cfunction(L,rt_add_viewport);
        } else if (key=="numViewports") {
                lua_pushnumber(L,rt->getNumViewports());
        } else if (key=="viewports") {
                lua_newtable(L);
                for (unsigned int i=0 ; i<rt->getNumViewports() ; i++) {
                        push_viewport(L,rt->getViewport(i));
                        lua_rawseti(L,-2,i+LUA_ARRAY_BASE);
                }
        } else if (key=="priority") {
                lua_pushnumber(L,rt->getPriority());
        } else if (key=="isActive") {
                lua_pushboolean(L,rt->isActive());
        } else if (key=="writeContentsToFile") {
                push_cfunction(L,rt_write_contents_to_file);
        } else if (key=="triangles") {
                lua_pushnumber(L,rt->getTriangleCount());
        } else if (key=="batches") {
                lua_pushnumber(L,rt->getBatchCount());
        } else if (key=="isPrimary") {
                lua_pushboolean(L,rt->isPrimary());
        } else if (key=="type") {
                if (dynamic_cast<Ogre::RenderWindow*>(rt)) {
                        lua_pushstring(L,"RenderWindow");
                } else if (dynamic_cast<Ogre::RenderTexture*>(rt)) {
                        lua_pushstring(L,"RenderTexture");
                } else {
                        my_lua_error(L,"Unrecognised Render Target.");
                }
        } else {
                return false;
        }
        return true;
TRY_END
}


static bool rt_newindex(lua_State *L, Ogre::RenderTarget *rt,
                        const std::string& key)
{
TRY_START
        if (key=="priority") {
                unsigned char v = check_t<unsigned char>(L,3);
                rt->setPriority(v);
        } else if (key=="isActive") {
                bool v = 0!=lua_toboolean(L,3);
                rt->setActive(v);
        } else {
                return false;
        }
        return true;
TRY_END
}

//}}}


// RENDER TEXTURE ========================================================== {{{

void push_rtex (lua_State *L, Ogre::RenderTexture *self)
{
        if (!self) {
                lua_pushnil(L);
                return;
        }
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (self);
        luaL_getmetatable(L, RTEX_TAG);
        lua_setmetatable(L, -2);
        render_target_maps& maps = grit->getUserDataTables().rts[self];
        maps.userData.push_back(ud);
}

static int rtex_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::RenderTexture,self,1,RTEX_TAG,0);
        if (self==NULL) return 0;
        vec_nullify_remove(grit->getUserDataTables().rts[self].userData,&self);
        return 0;
TRY_END
}


TOSTRING_GETNAME_MACRO(rtex,Ogre::RenderTexture,.getName(),RTEX_TAG)


static int rtex_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::RenderTexture,self,1,RTEX_TAG);
        std::string key = luaL_checkstring(L,2);
        if (rt_index(L,&self,key)) {
        } else {
                my_lua_error(L,"Not a valid RenderTexture member: "+key);
        }
        return 1;
TRY_END
}

static int rtex_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::RenderTexture,self,1,RTEX_TAG);
        std::string key = luaL_checkstring(L,2);
        if (rt_newindex(L,&self,key)) {
        } else {
                my_lua_error(L,"Not a valid RenderTexture member: "+key);
        }
        return 1;
TRY_END
}

EQ_PTR_MACRO(Ogre::RenderTexture,rtex,RTEX_TAG)

MT_MACRO_NEWINDEX(rtex);

//}}}


// RENDER WINDOW =========================================================== {{{

void push_rwin (lua_State *L, Ogre::RenderWindow *self)
{
        if (!self) {
                lua_pushnil(L);
                return;
        }
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (self);
        luaL_getmetatable(L, RWIN_TAG);
        lua_setmetatable(L, -2);
        render_target_maps& maps = grit->getUserDataTables().rts[self];
        maps.userData.push_back(ud);
}

static int rwin_set_fullscreen (lua_State *L)
{
TRY_START
        check_args(L,4);
        GET_UD_MACRO(Ogre::RenderWindow,self,1,RWIN_TAG);
        bool b = 0!=lua_toboolean(L,2);
        unsigned w = check_t<unsigned>(L,3);
        unsigned h = check_t<unsigned>(L,4);
        self.setFullscreen(b,w,h);
        return 0;
TRY_END
}

static int rwin_resize (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::RenderWindow,self,1,RWIN_TAG);
        unsigned w = check_t<unsigned>(L,3);
        unsigned h = check_t<unsigned>(L,4);
        self.resize(w,h);
        return 0;
TRY_END
}

static int rwin_get_metrics (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::RenderWindow,self,1,RWIN_TAG);
        unsigned int w, h, d;
        int l, t;
        self.getMetrics(w,h,d,l,t);
        lua_pushnumber(L,w);
        lua_pushnumber(L,h);
        lua_pushnumber(L,d);
        lua_pushnumber(L,l);
        lua_pushnumber(L,t);
        return 5;
TRY_END
}

static int rwin_reposition (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::RenderWindow,self,1,RWIN_TAG);
        unsigned x = check_t<unsigned>(L,2);
        unsigned y = check_t<unsigned>(L,3);
        self.reposition(x,y);
        return 0;
TRY_END
}

static int rwin_gc (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::RenderWindow,self,1,RWIN_TAG,0);
        if (self==NULL) return 0;
        vec_nullify_remove(grit->getUserDataTables().rts[self].userData,&self);
        return 0;
TRY_END
}


TOSTRING_GETNAME_MACRO(rwin,Ogre::RenderWindow,.getName(),RWIN_TAG)


static int rwin_index (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::RenderWindow,self,1,RWIN_TAG);
        std::string key = luaL_checkstring(L,2);
        if (rt_index(L,&self,key)) {
        } else if (key=="setFullScreen") {
                push_cfunction(L,rwin_set_fullscreen);
        } else if (key=="resize") {
                push_cfunction(L,rwin_resize);
        } else if (key=="getMetrics") {
                push_cfunction(L,rwin_get_metrics);
        } else if (key=="reposition") {
                push_cfunction(L,rwin_reposition);
        } else if (key=="isFullScreen") {
                lua_pushboolean(L,self.isFullScreen());
        } else if (key=="isActive") {
                lua_pushboolean(L,self.isActive());
        } else if (key=="autoDeactivate") {
                lua_pushboolean(L,self.isDeactivatedOnFocusChange());
        } else if (key=="visible") {
                lua_pushboolean(L,self.isVisible());
        } else {
                my_lua_error(L,"Not a valid RenderWindow member: "+key);
        }
        return 1;
TRY_END
}

static int rwin_newindex (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::RenderWindow,self,1,RWIN_TAG);
        std::string key = luaL_checkstring(L,2);
        if (rt_index(L,&self,key)) {
        } else if (key=="visible") {
                bool b = 0!=lua_toboolean(L,3);
                self.setVisible(b);
        } else if (key=="autoDeactivate") {
                bool b = 0!=lua_toboolean(L,3);
                self.setDeactivateOnFocusChange(b);
        } else {
                my_lua_error(L,"Not a valid RenderWindow member: "+key);
        }
        return 0;
TRY_END
}

EQ_PTR_MACRO(Ogre::RenderWindow,rwin,RWIN_TAG)

MT_MACRO_NEWINDEX(rwin);

//}}}

// vim: shiftwidth=8:tabstop=8:expandtab
