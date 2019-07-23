<?php
/**
 * @file
 * Provide Drush command to export information (sitename, nbr node) in CSV format.
 */

/**
 * Implements hook_drush_help().
 */
function nexteuropa_cck_export_drush_help($section)
{
  switch ($section) {
    case 'drush:ne-cck-export':
      return dt('Export CCK information in CSV format');
  }
}

/**
 * Implements hook_drush_command().
 */
function nexteuropa_cck_export_drush_command()
{
  $items = array();
  $items['ne-cck-export'] = array(
  'description' => 'tt',
  'options' => array(
    'result-file' => 'Write CSV output to file',
    'csv-separator' => 'Separator used for as CSV column delimiter',
    'values-separator' => 'Separator used for multiple values fields(eg languages list)',
    'no-header' => 'Do not print the header',
    'format' => 'Do not print the header',
  ),
  'examples' => array(
    'drush nccke ' => 'Nexteuropa CCK Export',
  ),
  'aliases' => array('nccke'),
  );
  return $items;
}

/**
 * Command callback. Remove file extension from drupal variable and field.
 */
function drush_nexteuropa_cck_export_ne_cck_export()
{
  $VALUES_SEPARATOR = drush_get_option('values-separator', ";");
  $CSV_SEPARATOR = drush_get_option('csv-separator', ",");

  $file = drush_get_option('result-file', false);

  if ($file && ((file_exists($file) && !is_writable($file)) || !is_writable(dirname($file)))) {
    return drush_set_error('DRUSH_NED_DEST_FILE_NOT_WRITEABLE', dt('Destination !file is not writable.', array('!file' => $file)));
  }

  global $conf;
  $result = array();

  $datas = array(
    "Site Id" => str_replace('sites/', '', conf_path()),
    "nexteuropa_cookie_consent_kit" => db_query("select status as module_enabled from {system} where name ='nexteuropa_cookie_consent_kit'")->fetchField(),
    "nexteuropa_cookie_consent_kit_display_cookie_banner" => variable_get('nexteuropa_cookie_consent_kit_display_cookie_banner', '0'),
    "nexteuropa_cookie_consent_kit_display_cookie_video" => variable_get('nexteuropa_cookie_consent_kit_display_cookie_video', '0'),
  );

  if ($file) {
    if(file_exists($file) == false && drush_get_option('no-header', false) == false) {
        drush_file_append_data($file, implode(array_keys($datas), $CSV_SEPARATOR) . PHP_EOL);
    }
    drush_file_append_data($file, implode($datas, $CSV_SEPARATOR) . PHP_EOL);
    drush_log("CSV generated " . $file, 'status');
  } else {
    switch (drush_get_option('format', false)) {
      case 'silent':
      case 'none':
        break;
      case 'list':
        foreach ($datas as $key => $data) {
            drush_print($key ." : ". $data );
            drush_print("----------------");
        }
        break;
      case 'CSV':
      default:
        drush_print(implode(array_keys($datas), $CSV_SEPARATOR));
        drush_print(implode($datas, $CSV_SEPARATOR));
        break;
    }
  }
}
