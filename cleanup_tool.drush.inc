<?php


/**
 * @file
 * Drush implementation for the cleanup_tool module.
 */

/**
 * Implementats hook_drush_command().
 */
function cleanup_tool_drush_command() {
  $items = array();
  $items['system-table-clean'] = array(
    'description' => 'Clean orphaned modules, these modules exist in the system table but are no longer on the filesystem: Experts only, use at your own risk',
  );
  $items['system-table-check'] = array(
    'arguments' => array(),
    'description' => 'Show all modules that are orphaned (files not on filesystem and was not pm_uninstalled and still has record in system table.)',
  );
  return $items;
}

/**
 * Drush command callback.
 */
function drush_cleanup_tool_system_table_clean() {
  drush_print(dt("Deleting all the above orphaned records from the system table. To see a list run 'drush system-table-check'."));
    //load all rows from system table
  $result = db_query("SELECT * FROM {system}");

  $items = array();
  foreach ($result as $record) {
    $record->info = unserialize($record->info);
    $items[$record->name] = $record;
  }

  $b_success=FALSE;
  foreach ($items as $item) {
    //Loop through all rows in system table, next line checks to see if the file exists

    if (!file_exists($item->filename)) {
      drush_print('Deleting the following record from the system table:');
      drush_print('name           = ' . $item->name);
      drush_print('type           = ' . $item->type);
      drush_print('filename       = ' . $item->filename);
      drush_print('Owner          = ' . $item->owner);
      drush_print('Bootstrap      = ' . $item->bootstrap);
      drush_print('schema_version = ' . $item->schema_version);
      drush_print('weight         = ' . $item->weight);
      $b_success = db_delete('system')
                  ->condition('filename', $item->filename)
                  ->execute();
      watchdog('cleanup_tool debug',
              '<pre>%item_array</pre> DELETE FROM {system} WHERE filename = %filename return value=%return_value',
              array('%item_array' => $itemi, '%filename' => $item->filename, '%return_value' => $b_success)
      );
    }
  }

  if ($b_success) {
    drush_cleanup_tool_system_table_check();
    drush_print("operation successful, " . $args[0] . " DELETED from system table" );
    return $b_success;
  } 
  else {
    drush_cleanup_tool_system_table_check();
    return $DRUSH_SUCCESS;
  }

}

/**
 * Get a list of all modules that are orphaned.
 */
function drush_cleanup_tool_system_table_check() {
  $query = db_select('system', 's');
  $query->fields('s', array('filename','name','type','owner','status','bootstrap','schema_version','weight' ))
        ->orderBy('type', 'DESC');

  $result = $query->execute();

  $b_clean=TRUE;
    drush_print('');
    drush_print('Check for modules that have not been properly uninstalled');
    drush_print('');
  while($record = $result->fetchObject()) {
    if (!file_exists($record->filename)) {
      if ($b_clean) {
        drush_print('');
        drush_print('Modules that have not been properly uninstalled:');
      }
      $b_clean=FALSE;
      drush_print('');
      drush_print('name           = ' . $record->name);
      drush_print('type           = ' . $record->type);
      drush_print('filename       = ' . $record->filename);
      drush_print('Owner          = ' . $record->owner);
      drush_print('Bootstrap      = ' . $record->bootstrap);
      drush_print('schema_version = ' . $record->schema_version);
      drush_print('weight         = ' . $record->weight);
    }
  }
  if ($b_clean) {
    drush_print('Installation is clean');
  }
  else {
    drush_print('Installations performance is affected.');
  }
  // @todo Set correct response code for Gregory's script :-)
  // And get rid of all this ugly code duplication...
}
