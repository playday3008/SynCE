#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnome.h>
#include <glade/glade.h>
#include <rapi.h>
#include <rra/matchmaker.h>

#include "device-info.h"


enum
{
  CURRENT_COLUMN,
  INDEX_COLUMN,
  ID_COLUMN,
  NAME_COLUMN,
  N_COLUMNS
};


static GladeXML *xml;


/* partnership */

static void
partners_create_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  guint32 index;
  gint id;
  gchar *name;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *device_info_dialog = glade_xml_get_widget (xml, "device_info_dialog");	
  GtkWidget *partners_list_view = glade_xml_get_widget (xml, "partners_list");	
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list_view));

  g_debug("%s: create button_clicked", G_STRFUNC);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  INDEX_COLUMN, &index,
			  ID_COLUMN, &id,
			  NAME_COLUMN, &name,
			  -1);
    }
  else
    {
      g_warning("%s: Failed to get selection", G_STRFUNC);
      return;
    }

  g_free (name);

  RRA_Matchmaker* matchmaker = NULL;

  if (!(matchmaker = rra_matchmaker_new())) {
    g_critical("%s: Failed to create match-maker", G_STRFUNC);
    return;
  }

  if (rra_matchmaker_create_partnership(matchmaker, &index)) {
    g_debug("%s: Partnership creation succeeded for index %d", G_STRFUNC, index);
  } else {
    g_warning("%s: Partnership creation failed for index %d", G_STRFUNC, index);
    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_WARNING,
						     GTK_BUTTONS_OK,
						     "Creation of a new partnership was unsuccessful");

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);
  }
  rra_matchmaker_destroy(matchmaker);
}

static void
partners_remove_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  gint index, id;
  gchar *name;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *partners_list_view = glade_xml_get_widget (xml, "partners_list");	
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list_view));

  g_debug("%s: remove button_clicked", G_STRFUNC);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  INDEX_COLUMN, &index,
			  ID_COLUMN, &id,
			  NAME_COLUMN, &name,
			  -1);
    }
  else
    {
      g_warning("%s: Failed to get selection", G_STRFUNC);
      return;
    }

  GtkWidget *device_info_dialog = glade_xml_get_widget (xml, "device_info_dialog");
  GtkWidget *confirm_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_QUESTION,
						     GTK_BUTTONS_YES_NO,
						     "Are you sure you want to remove the partnership ID %d with host %s ?",
						     id, name);

  gint result = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
  gtk_widget_destroy (confirm_dialog);
  g_free (name);
  switch (result)
    {
    case GTK_RESPONSE_YES:
      break;
    default:
      return;
      break;
    }

  RRA_Matchmaker* matchmaker = NULL;

  if (!(matchmaker = rra_matchmaker_new())) {
    g_critical("%s: Failed to create match-maker", G_STRFUNC);
    return;
  }

  if (rra_matchmaker_clear_partnership(matchmaker, index)) {
    g_debug("%s: Partnership cleaning succeeded for index %d", G_STRFUNC, index);
  } else {
    g_warning("%s: Partnership cleaning failed for index %d", G_STRFUNC, index);
  }
  rra_matchmaker_destroy(matchmaker);
}


static void
partners_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gint index, id;
  gchar *name;
  GtkWidget *partners_create_button, *partners_remove_button;

  partners_create_button = glade_xml_get_widget (xml, "partners_create_button");	
  partners_remove_button = glade_xml_get_widget (xml, "partners_remove_button");	

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  INDEX_COLUMN, &index,
			  ID_COLUMN, &id,
			  NAME_COLUMN, &name,
			  -1);

      g_debug("%s: You selected index %d, id %d, name %s", G_STRFUNC, index, id, name);
      g_free (name);

      if (id == 0) {
	gtk_widget_set_sensitive(partners_create_button, TRUE);
	gtk_widget_set_sensitive(partners_remove_button, FALSE);
      } else {
	gtk_widget_set_sensitive(partners_create_button, FALSE);
	gtk_widget_set_sensitive(partners_remove_button, TRUE);
      }

    }
}


static void
partners_setup_view_store(WmDevice *device)
{
  GtkWidget *partners_list_view = glade_xml_get_widget (xml, "partners_list");	
  GtkTreeIter iter;
  RRA_Matchmaker* matchmaker = NULL;
  guint32 curr_partner = 0, partner_id, i;
  gchar *partner_name;
  gboolean is_current;
  guint32 os_major = 0;

  GtkListStore *store = gtk_list_store_new (N_COLUMNS,
					    G_TYPE_BOOLEAN, /* current partner ? */
					    G_TYPE_INT,     /* partnee index */
					    G_TYPE_UINT,    /* partner id */
					    G_TYPE_STRING); /* partner name */

  /* WM5 not yet supported */
  g_object_get(device, "os-major", &os_major, NULL);
  if (os_major > 4) {
    goto exit;
  }

  wm_device_rapi_select(device);

  if (!(matchmaker = rra_matchmaker_new())) {
    g_critical("%s: Failed to create match-maker", G_STRFUNC);
    goto exit;
  }
  rra_matchmaker_get_current_partner(matchmaker, &curr_partner);

  for (i = 1; i <= 2; i++)
    {
      gtk_list_store_append (store, &iter);  /* Acquire an iterator */

      if (!(rra_matchmaker_get_partner_id(matchmaker, i, &partner_id))) {
	g_critical("%s: Failed to get partner %d id", G_STRFUNC, i);
	continue;
      }

      if (!(rra_matchmaker_get_partner_name(matchmaker, i, &partner_name))) {
	g_critical("%s: Failed to get partner %d name", G_STRFUNC, i);
	continue;
      }

      if (i == curr_partner)
	is_current = TRUE;
      else
	is_current = FALSE;

      gtk_list_store_set (store, &iter,
			  CURRENT_COLUMN, is_current,
			  INDEX_COLUMN, i,
			  ID_COLUMN, partner_id,
			  NAME_COLUMN, partner_name,
			  -1);

      rra_matchmaker_free_partner_name(partner_name);
    }

exit:
  gtk_tree_view_set_model (GTK_TREE_VIEW(partners_list_view), GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  if (matchmaker) rra_matchmaker_destroy(matchmaker);

  return;
}


static void
partners_setup_view(WmDevice *device)
{
  GtkWidget *partners_create_button, *partners_remove_button, *partners_list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  partners_create_button = glade_xml_get_widget (xml, "partners_create_button");
  partners_remove_button = glade_xml_get_widget (xml, "partners_remove_button");	
  partners_list = glade_xml_get_widget (xml, "partners_list");

  gtk_widget_set_sensitive(partners_create_button, FALSE);
  gtk_widget_set_sensitive(partners_remove_button, FALSE);

  g_signal_connect (G_OBJECT (partners_create_button), "clicked",
		    G_CALLBACK (partners_create_button_clicked_cb), device);
  g_signal_connect (G_OBJECT (partners_remove_button), "clicked",
		    G_CALLBACK (partners_remove_button_clicked_cb), device);

  partners_setup_view_store(device);

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
  column = gtk_tree_view_column_new_with_attributes("C",
						    renderer,
						    "active", CURRENT_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Index",
						    renderer,
						    "text", INDEX_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Partner Id",
						    renderer,
						    "text", ID_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Partner Name",
						    renderer,
						    "text", NAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);


  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (partners_selection_changed_cb),NULL);

  gtk_widget_show (partners_list);
}


/* system info */

/* from librapi2 pstatus */
static const char*
version_string(CEOSVERSIONINFO* version)
{
  const char* result = "Unknown";

  if (version->dwMajorVersion == 4)
    {
      if (version->dwMinorVersion == 20 && version->dwBuildNumber == 1081)
	result = "Ozone: Pocket PC 2003 (?)";
      else if (version->dwMinorVersion == 21 && version->dwBuildNumber == 1088)
	result = "Microsoft Windows Mobile 2003 Pocket PC Phone Edition";
    }
  else if (version->dwMajorVersion == 3 &&
	   version->dwMinorVersion == 0)
    {
      switch (version->dwBuildNumber)
	{
	case 9348:  result = "Rapier: Pocket PC"; break;
	case 11171: result = "Merlin: Pocket PC 2002"; break;

	  /*
	   * From:     Jonathan McDowell
	   * To:       SynCE-Devel
	   * Subject:  Re: [Synce-devel] Smartphone & installing CABs.
	   * Date:     Mon, 26 May 2003 19:12:10 +0100  (20:12 CEST)
	   */
	case 12255: result = "Stinger: Smart Phone 2002"; break;

	  /* My Qtek 7070 */
	case 13121: result = "Stinger: Smart Phone 2002"; break;
	}
    }
  else if (version->dwMajorVersion == 2 &&
	   version->dwMinorVersion == 1)
    {
    result =
      "Gryphon: Windows CE for P/PC V1 (Palm-size PC)"
      " / "
      "Apollo: Windows CE for A/PC V1 (Auto PC)";
    }

  return result;
}

#define PROCESSOR_ARCHITECTURE_COUNT 8

static const char* architecture[] = {
  "Intel",
  "MIPS",
  "Alpha",
  "PPC",
  "SHX",
  "ARM",
  "IA64",
  "ALPHA64"
};


static const gchar*
processor(gint n)
{
  const gchar* result;

  switch (n)
    {
    case PROCESSOR_STRONGARM:    result = "StrongARM";  break;
    case PROCESSOR_MIPS_R4000:   result = "MIPS R4000"; break;
    case PROCESSOR_HITACHI_SH3:  result = "SH3";        break;

    default:
      result = "Unknown";
      g_debug("%s: Unknown processor type, please send your device details to synce-devel@lists.sourceforge.net", G_STRFUNC);
      break;
    }

  return result;
}


static void
system_info_setup_view_store(WmDevice *device)
{
  GtkWidget *sys_info_store_size, *sys_info_store_free,
    *sys_info_store_ram, *sys_info_store_storage;
  STORE_INFORMATION store;
  DWORD storage_pages = 0, ram_pages = 0, page_size = 0;

  memset(&store, 0, sizeof(store));

  sys_info_store_size = glade_xml_get_widget (xml, "sys_info_store_size");
  sys_info_store_free = glade_xml_get_widget (xml, "sys_info_store_free");
  sys_info_store_ram = glade_xml_get_widget (xml, "sys_info_store_ram");
  sys_info_store_storage = glade_xml_get_widget (xml, "sys_info_store_storage");

  if (CeGetStoreInformation(&store)) {
    gchar *store_size = g_strdup_printf("%i bytes (%i MB)", store.dwStoreSize, store.dwStoreSize / (1024*1024));
    gchar *free_space = g_strdup_printf("%i bytes (%i MB)", store.dwFreeSize,  store.dwFreeSize  / (1024*1024));

    gtk_label_set_text(GTK_LABEL(sys_info_store_size), store_size);
    gtk_label_set_text(GTK_LABEL(sys_info_store_free), free_space);

    g_free(store_size);
    g_free(free_space);
  } else {
    g_warning("%s: Failed to get store information: %s",
	      G_STRFUNC,
	      synce_strerror(CeGetLastError()));
  }

  if (CeGetSystemMemoryDivision(&storage_pages, &ram_pages, &page_size)) {
    gchar *storage_size = g_strdup_printf("%i bytes (%i MB)", storage_pages * page_size, storage_pages * page_size / (1024*1024));
    gchar *ram_size = g_strdup_printf("%i bytes (%i MB)", ram_pages * page_size, ram_pages * page_size / (1024*1024));

    gtk_label_set_text(GTK_LABEL(sys_info_store_ram), ram_size);
    gtk_label_set_text(GTK_LABEL(sys_info_store_storage), storage_size);

    g_free(storage_size);
    g_free(ram_size);
  }
}

static void
system_info_setup_view(WmDevice *device)
{
  GtkWidget *sys_info_model, *sys_info_version, *sys_info_platform, *sys_info_details,
    *sys_info_proc_arch, *sys_info_proc_type, *sys_info_page_size;
  CEOSVERSIONINFO version;
  SYSTEM_INFO system;
  gchar *class, *hardware, *model_str;

  wm_device_rapi_select(device);

  sys_info_model = glade_xml_get_widget (xml, "sys_info_model");
  g_object_get(device, "class", &class, NULL);
  g_object_get(device, "hardware", &hardware, NULL);
  model_str = g_strdup_printf("%s (%s)", hardware, class);
  gtk_label_set_text(GTK_LABEL(sys_info_model), model_str);
  g_free(class);
  g_free(hardware);
  g_free(model_str);

  /* Version */

  sys_info_version = glade_xml_get_widget (xml, "sys_info_version");
  sys_info_platform = glade_xml_get_widget (xml, "sys_info_platform");
  sys_info_details = glade_xml_get_widget (xml, "sys_info_details");

  memset(&version, 0, sizeof(version));
  version.dwOSVersionInfoSize = sizeof(version);

  if (CeGetVersionEx(&version)) {
    char *details = wstr_to_current(version.szCSDVersion);
    char *platform = NULL;

    if (VER_PLATFORM_WIN32_CE == version.dwPlatformId)
      platform = "(Windows CE)";

    gchar *version_str = g_strdup_printf("%i.%i.%i (%s)",
					 version.dwMajorVersion,
					 version.dwMinorVersion,
					 version.dwBuildNumber,
					 version_string(&version));

    gchar *platform_str = g_strdup_printf("%i %s",
					  version.dwPlatformId,
					  platform ? platform : "");

    gtk_label_set_text(GTK_LABEL(sys_info_version), version_str);
    gtk_label_set_text(GTK_LABEL(sys_info_platform), platform_str);
    gtk_label_set_text(GTK_LABEL(sys_info_details), details);

    g_free(version_str);
    g_free(platform_str);
    wstr_free_string(details);
  } else {
    g_warning("%s: Failed to get version information: %s",
	       G_STRFUNC,
	       synce_strerror(CeGetLastError()));
  }

  /* platform */

  sys_info_proc_arch = glade_xml_get_widget (xml, "sys_info_proc_arch");
  sys_info_proc_type = glade_xml_get_widget (xml, "sys_info_proc_type");
  sys_info_page_size = glade_xml_get_widget (xml, "sys_info_page_size");

  memset(&system, 0, sizeof(system));

  CeGetSystemInfo(&system);
  {
    gchar *proc_arch_str = g_strdup_printf("%i (%s)",
					   system.wProcessorArchitecture,
					   (system.wProcessorArchitecture < PROCESSOR_ARCHITECTURE_COUNT) ?
					   architecture[system.wProcessorArchitecture] : "Unknown");

    gchar *proc_type_str = g_strdup_printf("%i (%s)",
					   system.dwProcessorType,
					   processor(system.dwProcessorType));

    gchar *page_size_str = g_strdup_printf("0x%x",
					   system.dwAllocationGranularity);

    gtk_label_set_text(GTK_LABEL(sys_info_proc_arch), proc_arch_str);
    gtk_label_set_text(GTK_LABEL(sys_info_proc_type), proc_type_str);
    gtk_label_set_text(GTK_LABEL(sys_info_page_size), page_size_str);

    g_free(proc_arch_str);
    g_free(proc_type_str);
    g_free(page_size_str);
  }

  /* store */

  system_info_setup_view_store(device);

}


/* power */

static const gchar*
get_ACLineStatus_string(unsigned ACLineStatus)
{
  const gchar* status;

  switch (ACLineStatus)
    {
    case AC_LINE_OFFLINE:       status = "Offline";       break;
    case AC_LINE_ONLINE:        status = "Online";        break;
    case AC_LINE_BACKUP_POWER:  status = "Backup Power";  break;
    case AC_LINE_UNKNOWN:       status = "Unknown";       break;
    default:                    status = "Invalid";       break;
    }

  return status;
}

static const gchar*
get_battery_flag_string(unsigned flag)
{
  const gchar* name;

  switch (flag)
    {
    case BATTERY_FLAG_HIGH:        name = "High";       break;
    case BATTERY_FLAG_LOW:         name = "Low";        break;
    case BATTERY_FLAG_CRITICAL:    name = "Critical";   break;
    case BATTERY_FLAG_CHARGING:    name = "Charging";   break;
    case BATTERY_FLAG_NO_BATTERY:  name = "No Battery"; break;

    default: name = "Unknown"; break;
    }

  return name;
}


static void
system_power_setup_view(WmDevice *device)
{
  SYSTEM_POWER_STATUS_EX power;
  GtkWidget *sys_info_ac_status,
    *sys_info_main_batt_lifetime, *sys_info_main_batt_fulllife, *sys_info_main_batt_bar,
    *sys_info_bup_batt_lifetime, *sys_info_bup_batt_fulllife, *sys_info_bup_batt_bar;
  gchar *lifetime, *fulllife, *lifepercent;

  sys_info_ac_status = glade_xml_get_widget (xml, "sys_info_ac_status");

  sys_info_main_batt_lifetime = glade_xml_get_widget (xml, "sys_info_main_batt_lifetime");
  sys_info_main_batt_fulllife = glade_xml_get_widget (xml, "sys_info_main_batt_fulllife");
  sys_info_main_batt_bar = glade_xml_get_widget (xml, "sys_info_main_batt_bar");

  sys_info_bup_batt_lifetime = glade_xml_get_widget (xml, "sys_info_bup_batt_lifetime");
  sys_info_bup_batt_fulllife = glade_xml_get_widget (xml, "sys_info_bup_batt_fulllife");
  sys_info_bup_batt_bar = glade_xml_get_widget (xml, "sys_info_bup_batt_bar");

  memset(&power, 0, sizeof(SYSTEM_POWER_STATUS_EX));

  if (!(CeGetSystemPowerStatusEx(&power, false))) {
    g_warning("%s: Failed to get battery status: %s",
	      G_STRFUNC,
	      synce_strerror(CeGetLastError()));
    return;
  }

  gchar *acstatus = g_strdup_printf("%02x (%s)", power.ACLineStatus, get_ACLineStatus_string(power.ACLineStatus));
  gtk_label_set_text(GTK_LABEL(sys_info_ac_status), acstatus);
  g_free(acstatus);

  if (BATTERY_LIFE_UNKNOWN == power.BatteryLifeTime)
    lifetime = g_strdup("Unknown");
  else
    lifetime = g_strdup_printf("%i", power.BatteryLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_main_batt_lifetime), lifetime);
  g_free(lifetime);

  if (BATTERY_LIFE_UNKNOWN == power.BatteryFullLifeTime)
    fulllife = g_strdup("Unknown");
  else
    fulllife = g_strdup_printf("%i", power.BatteryFullLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_main_batt_fulllife), fulllife);
  g_free(fulllife);

  if (BATTERY_PERCENTAGE_UNKNOWN == power.BatteryLifePercent) {
    lifepercent = g_strdup_printf("%s (%% Unknown)", get_battery_flag_string(power.BatteryFlag));
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_main_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_main_batt_bar), 0.0);
  } else {
    lifepercent = g_strdup_printf("%s (%i%%)", get_battery_flag_string(power.BatteryFlag), power.BatteryLifePercent);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_main_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_main_batt_bar), (gdouble)(power.BatteryLifePercent / 100.0));
  }
  g_free(lifepercent);

  if (BATTERY_LIFE_UNKNOWN == power.BackupBatteryLifeTime)
    lifetime = g_strdup("Unknown");
  else
    lifetime = g_strdup_printf("%i", power.BackupBatteryLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_bup_batt_lifetime), lifetime);
  g_free(lifetime);

  if (BATTERY_LIFE_UNKNOWN == power.BackupBatteryFullLifeTime)
    fulllife = g_strdup("Unknown");
  else
    fulllife = g_strdup_printf("%i", power.BackupBatteryFullLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_bup_batt_fulllife), fulllife);
  g_free(fulllife);

  if (BATTERY_PERCENTAGE_UNKNOWN == power.BackupBatteryLifePercent) {
    lifepercent = g_strdup_printf("%s (%% Unknown)", get_battery_flag_string(power.BackupBatteryFlag));
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), 0.0);
  } else {
    lifepercent = g_strdup_printf("%s (%i%%)", get_battery_flag_string(power.BackupBatteryFlag), power.BackupBatteryLifePercent);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), (gdouble)(power.BackupBatteryLifePercent / 100.0));
  }
  g_free(lifepercent);

}


static void
device_info_refresh_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  WmDevice *device = WM_DEVICE(data);

  partners_setup_view_store(device);
  system_info_setup_view_store(device);
  system_power_setup_view(device);
}


static void
device_info_close_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}

GtkWidget *
run_device_info_dialog (WmDevice *device)
{
  GtkWidget *device_info_dialog, *device_info_dialog_close, *device_info_dialog_refresh;
  gchar *device_name, *title;

  xml = glade_xml_new (SYNCE_DATA "synce_trayicon_properties.glade", "device_info_dialog", NULL);

  device_info_dialog = glade_xml_get_widget (xml, "device_info_dialog");
  device_info_dialog_close = glade_xml_get_widget (xml, "device_info_dialog_close");
  device_info_dialog_refresh = glade_xml_get_widget (xml, "device_info_dialog_refresh");

  g_signal_connect (G_OBJECT (device_info_dialog_close), "clicked",
		    G_CALLBACK (device_info_close_button_clicked_cb), NULL);

  g_signal_connect (G_OBJECT (device_info_dialog_refresh), "clicked",
		    G_CALLBACK (device_info_refresh_button_clicked_cb), device);

  g_object_get(device, "name", &device_name, NULL);
  title = g_strdup_printf("%s Information", device_name);
  gtk_window_set_title(GTK_WINDOW(device_info_dialog), title);
  g_free(device_name);
  g_free(title);

  partners_setup_view(device);

  system_info_setup_view(device);

  system_power_setup_view(device);


  gtk_widget_show_all (device_info_dialog);

  return device_info_dialog;
}
