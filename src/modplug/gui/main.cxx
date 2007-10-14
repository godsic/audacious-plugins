/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <libintl.h>
extern "C" {
#include <audacious/util.h>
#include <audacious/strings.h>
}

#include "interface.h"
#include "support.h"
#include "main.h"

#include <sstream>
//open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//mmap()
#include <unistd.h>
#include <sys/mman.h>
#include <fstream>

#include "../stddefs.h"
#include "../stdafx.h"
#include "../sndfile.h"
#include "../archive/open.h"

#define MAX_MESSAGE_LENGTH 4000

GtkWidget *AboutWin    = NULL;
GtkWidget *ConfigWin   = NULL;
GtkWidget *InfoWin     = NULL;

void ShowAboutWindow()
{
	if(!AboutWin)
	{
		gchar * about_text = g_strjoin( "" , _("Modplug Input Plugin for Audacious ver") ,
				VERSION , _("\nModplug sound engine written by Olivier Lapicque.\nXMMS interface for Modplug by Kenton Varda.\n(c)2000 Olivier Lapicque and Kenton Varda.\nUpdates and Maintainance by Konstanty Bialkowski.\nPorted to BMP by Theofilos Intzoglou.") , NULL );
		AboutWin = audacious_info_dialog( _("About Modplug") , about_text , _("Ok") , FALSE , NULL , NULL );
		gtk_signal_connect( GTK_OBJECT(AboutWin) , "destroy" ,
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &AboutWin);
		g_free( about_text );
	}
	gtk_widget_show(AboutWin);
}

void ShowConfigureWindow(const ModplugXMMS::Settings& aProps)
{
	if(!ConfigWin)
		ConfigWin = create_Config();
	
	if(aProps.mBits == 8)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "bit8"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "bit16"), TRUE);
	
	if(aProps.mFrequency == 11025)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "samp11"), TRUE);
	else if (aProps.mFrequency == 22050)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "samp22"), TRUE);
	else if (aProps.mFrequency == 48000)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "samp48"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "samp44"), TRUE);
	
	if(aProps.mChannels == 1)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "mono"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "stereo"), TRUE);
	
	if(aProps.mResamplingMode == 0)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "resampNearest"), TRUE);
	else if(aProps.mResamplingMode == 1)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "resampLinear"), TRUE);
	else if(aProps.mResamplingMode == 2)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "resampSpline"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "resampPolyphase"), TRUE);
	
	if(aProps.mNoiseReduction)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxNR"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxNR"), FALSE);
	
	if(aProps.mGrabAmigaMOD)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxAmigaMOD"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxAmigaMOD"), FALSE);
	
	if(aProps.mFastinfo)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxFastInfo"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxFastInfo"), FALSE);
	
	if(aProps.mUseFilename)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxUseFilename"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxUseFilename"), FALSE);
	
	if(aProps.mReverb)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxReverb"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxReverb"), FALSE);
	
	if(aProps.mMegabass)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxBassBoost"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxBassBoost"), FALSE);
	
	if(aProps.mSurround)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxSurround"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxSurround"), FALSE);
	
	if(aProps.mPreamp)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxPreamp"), TRUE);
	else
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxPreamp"), FALSE);
	
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxReverbDepth")),   aProps.mReverbDepth);
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxReverbDelay")),   aProps.mReverbDelay);
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxBassAmount")),    aProps.mBassAmount);
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxBassRange")),     aProps.mBassRange);
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxSurroundDepth")), aProps.mSurroundDepth);
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxSurroundDelay")), aProps.mSurroundDelay);
	gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange*)lookup_widget(ConfigWin, "fxPreampLevel")), aProps.mPreampLevel);
	
	if(aProps.mLoopCount < 0)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxLoopForever"), TRUE);
	else if(aProps.mLoopCount == 0)
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxNoLoop"), TRUE);
	else
	{
		gtk_toggle_button_set_active((GtkToggleButton*)lookup_widget(ConfigWin, "fxLoopFinite"), TRUE);
		gtk_adjustment_set_value(gtk_spin_button_get_adjustment(
			(GtkSpinButton*)lookup_widget(ConfigWin, "fxLoopCount")), aProps.mLoopCount);
	}
	
	gtk_widget_show(ConfigWin);
}

void ShowInfoWindow(const string& aFilename)
{
	if(!InfoWin)
		InfoWin = create_Info();

	uint32 lSongTime, lNumSamples, lNumInstruments, i;
	string lInfo;
	gchar lBuffer[33];
	stringstream lStrStream(ios::out);   //C++ replacement for sprintf()

	CSoundFile* lSoundFile;

	Archive* lArchive;
	string lShortFN;
	uint32 lPos;
	gchar *tmps;

	lPos = aFilename.find_last_of('/') + 1;
	lShortFN = aFilename.substr(lPos);

	//open and mmap the file
	lArchive = OpenArchive(aFilename);
	if(lArchive->Size() == 0)
	{
		delete lArchive;
		return;
	}

	lSoundFile = new CSoundFile;
	lSoundFile->Create((uchar*)lArchive->Map(), lArchive->Size());

	lInfo = lShortFN;
	lInfo += '\n';
	tmps = aud_str_to_utf8(lSoundFile->GetTitle());
	lInfo += tmps;
	g_free(tmps);
	lInfo += '\n';

	switch(lSoundFile->GetType())
	{
	case MOD_TYPE_MOD:
		lInfo+= "ProTracker";
		break;
	case MOD_TYPE_S3M:
		lInfo+= "Scream Tracker 3";
		break;
	case MOD_TYPE_XM:
		lInfo+= "Fast Tracker 2";
		break;
	case MOD_TYPE_IT:
		lInfo+= "Impulse Tracker";
		break;
	case MOD_TYPE_MED:
		lInfo+= "OctaMed";
		break;
	case MOD_TYPE_MTM:
		lInfo+= "MTM";
		break;
	case MOD_TYPE_669:
		lInfo+= "669 Composer / UNIS 669";
		break;
	case MOD_TYPE_ULT:
		lInfo+= "ULT";
		break;
	case MOD_TYPE_STM:
		lInfo+= "Scream Tracker";
		break;
	case MOD_TYPE_FAR:
		lInfo+= "Farandole";
		break;
	case MOD_TYPE_AMF:
		lInfo+= "ASYLUM Music Format";
		break;
	case MOD_TYPE_AMS:
		lInfo+= "AMS module";
		break;
	case MOD_TYPE_DSM:
		lInfo+= "DSIK Internal Format";
		break;
	case MOD_TYPE_MDL:
		lInfo+= "DigiTracker";
		break;
	case MOD_TYPE_OKT:
		lInfo+= "Oktalyzer";
		break;
	case MOD_TYPE_DMF:
		lInfo+= "Delusion Digital Music Fileformat (X-Tracker)";
		break;
	case MOD_TYPE_PTM:
		lInfo+= "PolyTracker";
		break;
	case MOD_TYPE_DBM:
		lInfo+= "DigiBooster Pro";
		break;
	case MOD_TYPE_MT2:
		lInfo+= "MT2";
		break;
	case MOD_TYPE_AMF0:
		lInfo+= "AMF0";
		break;
	case MOD_TYPE_PSM:
		lInfo+= "PSM";
		break;
	default:
		lInfo+= "Unknown";
		break;
	}
	lInfo += '\n';

	lSongTime = lSoundFile->GetSongTime();
	lStrStream.clear();
	lStrStream << (int)(lSongTime / 60) << ":";
	if(lSongTime % 60 < 10)  //single digit for seconds?
		lStrStream << '0';        //yes, so add a 0.
	lStrStream << (int)(lSongTime % 60);

	lStrStream << '\n';

	lStrStream << (int)lSoundFile->GetMusicSpeed() << '\n';
	lStrStream << (int)lSoundFile->GetMusicTempo() << '\n';
	lStrStream << (int)(lNumSamples = lSoundFile->GetNumSamples()) << '\n';
	lStrStream << (int)(lNumInstruments = lSoundFile->GetNumInstruments());
	lStrStream << '\n';
	lStrStream << (int)(lSoundFile->GetNumPatterns()) << '\n';
	lStrStream << (int)lSoundFile->GetNumChannels();
	lInfo += lStrStream.str();

	gtk_label_set_text((GtkLabel*)lookup_widget(InfoWin, "info_general"), lInfo.c_str());

	lInfo = "";
	for(i = 0; i < lNumSamples; i++)
	{
		lSoundFile->GetSampleName(i, lBuffer);
		tmps = aud_str_to_utf8(lBuffer);
		lInfo += tmps;
		g_free(tmps);
		lInfo += '\n';
	}
	gtk_label_set_text((GtkLabel*)lookup_widget(InfoWin, "info_samples"), lInfo.c_str());

	lInfo = "";
	for(i = 0; i < lNumInstruments; i++)
	{
		lSoundFile->GetInstrumentName(i, lBuffer);
		tmps = aud_str_to_utf8(lBuffer);
		lInfo += tmps;
		g_free(tmps);
		lInfo += '\n';
	}
	gtk_label_set_text((GtkLabel*)lookup_widget(InfoWin, "info_instruments"), lInfo.c_str());

	char message[MAX_MESSAGE_LENGTH];
	static int length = 0;
	
	//textbox = (GtkLabel*)lookup_widget(InfoWin, "info_message");
	//gtk_text_backward_delete(textbox, length);
	length = lSoundFile->GetSongComments(message, MAX_MESSAGE_LENGTH, 80);
	if (length != 0) {
		tmps = aud_str_to_utf8(message);
		gtk_label_set_text((GtkLabel*)lookup_widget(InfoWin, "info_message"), tmps);
		g_free(tmps);
	}
	
	//unload the file
	lSoundFile->Destroy();
	delete lSoundFile;
	delete lArchive;

	gtk_widget_show(InfoWin);
}
