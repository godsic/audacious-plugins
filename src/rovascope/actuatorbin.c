#include "paranormal.h"
#include "actuators.h"

struct pn_actuator *
rovascope_get_random_colourmap(void)
{
   struct pn_actuator *out;

   srand(time(NULL));

   out = create_actuator("cmap_bwgradient");
   out->options[2].val.cval.r = 255 - rand() % 255;
   out->options[2].val.cval.g = 255 - rand() % 255;
   out->options[2].val.cval.b = 255 - rand() % 255;

   return out;
}

struct pn_actuator *
rovascope_get_random_general(void)
{
   struct pn_actuator *out;

   gchar *candidates[] = {
      "general_fade", "general_blur", "general_mosaic", 
      "general_flip",
   };

   out = create_actuator(candidates[rand() % G_N_ELEMENTS(candidates)]);

   return out;
}

struct pn_actuator *
rovascope_get_random_normal_scope(void)
{
   struct pn_actuator *out;
   static gint ret = 0;

   gchar *candidates[] = {
      "wave_horizontal", "wave_vertical", "wave_radial",
   };

   if (ret >= G_N_ELEMENTS(candidates))
       ret = 0;

   out = create_actuator(candidates[ret]);

   ret++;

   return out;
}

struct pn_actuator *
rovascope_get_random_actuator(void)
{
   struct pn_actuator *out;
   struct pn_actuator *(*func)();
   static gint ret = 0;

   void *candidates[] = {
	rovascope_get_random_colourmap,
	rovascope_get_random_general,
	rovascope_get_random_normal_scope,
   };

   if (ret >= G_N_ELEMENTS(candidates))
       ret = 0;   

   func = candidates[ret];
   out = func();

   ret++;

   return out;
}
