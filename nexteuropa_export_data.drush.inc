<?php
/**
 * @file
 * Provide Drush command to export information (sitename, nbr node) in CSV format.
 */

define("FEATURES", "features");
define("CUSTOM", "custom");
define("CONTRIB", "contrib");

/**
 * Implements hook_drush_help().
 */
function nexteuropa_export_data_drush_help($section)
{
    switch ($section) {
    case 'drush:ne-export-data':
        return dt('Export information (sitename, nbr node) in CSV format');
    }
}

/**
 * Implements hook_drush_command().
 */
function nexteuropa_export_data_drush_command()
{
    $items = array();
    $items['ne-export-data'] = array(
    'description' => '',
    // 'required-arguments' => 1,
    'options' => array(
      'result-file' => 'Write CSV output to file',
      'csv-separator' => 'Separator used for as CSV column delimiter',
      'values-separator' => 'Separator used for multiple values fields(eg languages list)',
      'no-header' => 'Do not print the header',
      'format' => 'Do not print the header',
      'no-cache' => 'No drush cache'
    ),
    'examples' => array(
      'drush NED ' => 'Nexteuropa Export Data',
    ),
    'aliases' => array('ned'),
    );
    return $items;
}

/**
 * Command callback. Remove file extension from drupal variable and field.
 */
function drush_nexteuropa_export_data_ne_export_data()
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
    "URL" => $GLOBALS['base_url'],
    "Site og:title" => nexteuropa_export_get_metatag("og:title"),
    "Site name" => variable_get('site_name', ''),
    "Site slogan" => '"' . variable_get('site_slogan', '') . '"',
    "Number of published nodes" => db_query("SELECT count(*) FROM {node} WHERE status=1")->fetchField(),
    "Number of unpublished nodes" => db_query("SELECT count(*) FROM {node} WHERE status=1")->fetchField(),
    "Number of files" => db_query("SELECT count(*) FROM {file_managed}")->fetchField(),
    "Number of videos" =>  db_query("SELECT count(*) FROM {file_managed} WHERE type='video'")->fetchField(),
    "Number of active internal users" => db_query(
        "SELECT count(uid) from {users} WHERE access >= :access AND mail NOT LIKE (:mail)",
        array(":access" => strtotime("-6 months"), ":mail" => "%@%europa.eu")
    )->fetchField(),
    "Number of active external users"=> db_query(
        "SELECT count(uid) from {users} WHERE access >= :access AND mail NOT LIKE (:mail)",
        array(":access" => strtotime("-6 months"), ":mail" => "%@%europa.eu")
    )->fetchField(),
    "Drupal version" => VERSION,
    "Drupal profile" => variable_get('install_profile', ''),
    "Nexteuropa version" => nexteuropa_export_data_get_master_version(),
    "Current theme" => variable_get('theme_default', ''),
    "Current admin theme" => variable_get('admin_theme', ''),
    "Number of content types" => count(nexteuropa_export_data_get_node_types()),
    "List of content types" => '"' . implode(nexteuropa_export_data_get_node_types(), $VALUES_SEPARATOR) . '"',
    "Languages Number" => count(nexteuropa_export_data_get_languages()),
    "Languages" => '"' . implode(nexteuropa_export_data_get_languages(), $VALUES_SEPARATOR) . '"'
    );

    // Add module number and list, by type (feature/contrib/custom).
    $modules_by_types = nexteuropa_export_data_get_modules_by_types();
    foreach ($modules_by_types as $type => $modules) {
        $idC= "Number of enabled " . $type . " modules";
        $datas[$idC] = count(array_keys($modules));
        $idL = "List of enabled " . $type . " modules";
        $datas[$idL] = implode(array_keys($modules), $VALUES_SEPARATOR);
    }

    // Contrib D8 modules
    $md8 = nexteuropa_export_data_get_modules_contrib_not_in_D8();
    $datas["Number contrib without a D8 version"] = count($md8);
    $datas["List contrib without a D8 version"] = implode(array_keys($md8), $VALUES_SEPARATOR);

    // Add metrics.
    $mTypes = array(CUSTOM,FEATURES);
    foreach ($mTypes as $mType) {
        $metricsTypes = nexteuropa_export_data_get_modules_metrics($modules_by_types[$mType]);
        foreach ($metricsTypes as $metricType => $metrics) {
            $datas[$mType . " - " . $metricType] = $metrics;
        }
    }

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

/**
 *  Get metatag from website
 */
function nexteuropa_export_data_get_master_version()
{
  $version_file = DRUPAL_ROOT . "/.version";
  if (file_exists ( $version_file)){
    return file_get_contents ($version_file);
  }
  return "";
}

/**
 *  Get metatag from website
 */
function nexteuropa_export_get_metatag($tag)
{
    $ogtitle="";

    $page = file_get_contents ($GLOBALS['base_url']);
    if(preg_match('/<meta property="'. $tag .'" content="(.*?)" \/>/', $page, $matches)) {
        $ogtitle = $matches[1];
    }
    return $ogtitle;
}

/**
 * Get list of node types
 */
function nexteuropa_export_data_get_node_types()
{
    static $node_types;
    if (!$node_types) {
        $node_types = db_query("SELECT type FROM {node_type}")->fetchAll(PDO::FETCH_COLUMN);
    }
    return $node_types;
}

/**
 * Get list of languages
 */
function nexteuropa_export_data_get_languages()
{
    static $languages;
    if (!$languages) {
        $languages = db_query("SELECT name FROM {languages}")->fetchAll(PDO::FETCH_COLUMN);
    }
    return $languages;
}

/**
 * Return array of contrib, custom, and features modules.
 * Core module not included
 */
function nexteuropa_export_data_get_modules_by_types()
{
    static $modules;
    if (!$modules) {
        $modules[CONTRIB] = [];
        $modules[FEATURES] = [];
        $modules[CUSTOM] = [];
    } else {
        return $modules;
    }

    $query = db_select('system', 's')->fields('s', array('name','status','info','filename'));
    $query->condition('type', 'module', '=');
    $query->condition('status', '1', '=');
    $query->condition('filename', 'modules/%', 'NOT LIKE');
    $modulesTmp = $query->execute()->fetchAll();

    foreach ($modulesTmp as $module) {
        $module->info = unserialize($module->info);
        if($type = _nexteuropa_export_data_get_module_type($module)) {
            $modules[$type][$module->name] = $module;
        }
    }
    return $modules;
}

/**
 * Determine module type
 */
function _nexteuropa_export_data_get_module_type($module)
{
    static $modules_cache;

    $cacheID = "ned-modules-type";
    if (!$modules_cache) {
        if (!drush_get_option('no-cache', false)) {
            $dataCache = drush_cache_get($cacheID);
            if (!empty($dataCache)) {
                drush_log("NED - Modules type retrieve from cache", 'debug');
                $modules_cache = $dataCache->data;
            }
        }
    }
    $version = (!empty($module->info['version'])) ? $module->info['version'] : "7.x";
    if (isset($modules_cache[$module->name])) {
        drush_log(dt("NED - Modules type retrieve from cache for !module", array('!module'=>$module->name)), 'debug');
        return $modules_cache[$module->name];
    }

    drush_log(dt("NED - Determine modules type for !module", array('!module'=>$module->name)), 'debug');
    $type=false;
    if (preg_match("/profiles\/*\/.*\.profile/", $module->filename) != 1) { // exclude profile
        // datestamp and project exist ? no 'custom' on filename, it seems to be a contrib...
        if (isset($module->info['datestamp']) &&
            isset($module->info['project']) &&
            strpos("/modules/custom/" , $module->filename) === false
        ) {
            // Ensure module exist on drupal.org. (remove sub-module)
            $drupal_module = nexteuropa_export_data_get_contrib_module_release_info($module->name);
            if (isset($drupal_module['title']) && !empty($drupal_module['title'])) {
                drush_log(dt("NED - !module exist on drupal.org, it's a contrib !", array('!module' =>$module->name)), 'debug');
                $type = CONTRIB;
            } else {
                drush_log(dt("NED - !module not found on drupal.org but have pachage information... a submodule ?", array('!module' =>$module->name)), 'debug');
            }
        } else {
            // remove features from module list
            if (isset($module->info['features'])) {
                drush_log(dt("NED - !module is a feature", array('!module' =>$module->name)), 'debug');
                $type = FEATURES;
            } else {
                drush_log(dt("NED - !module is a custom", array('!module' =>$module->name)), 'debug');
                $type = CUSTOM;
            }
        }
    }
    $modules_cache[$module->name] = $type;
    if (!drush_get_option('no-cache', false)) {
        drush_cache_set($cacheID, $modules_cache);
    }
    return $type;
}

/**
 *  Get sum of modules metrics
 */
function nexteuropa_export_data_get_modules_metrics($modules)
{
    $modulesMetrics = [];
    foreach ($modules as $key => $module) {
        $moduleMetrics = nexteuropa_export_data_get_module_metrics($module);
        if (!empty($moduleMetrics)) {
            foreach ($moduleMetrics as $type => $metric) {
                $modulesMetrics[$type] = isset($modulesMetrics[$type]) ? $modulesMetrics[$type] + $metric :  + $metric;
            }
        }
    }
    return $modulesMetrics;
}

/**
 *  Get metrics for a module
 */
function nexteuropa_export_data_get_module_metrics($module)
{
    $cache_enabled=!drush_get_option('no-cache', false);
    $sha1 = sha1_file($module->filename);
    $version = (!empty($module->info['version'])) ? $module->info['version'] : "7.x";
    if ($cache_enabled) {
        $cacheID = "ned-module-metrics-".$module->name. "-" . $version . "-" . $sha1;
        $dataCache = drush_cache_get($cacheID);
        if (!empty($dataCache)) {
            drush_log(dt("NED - Retrieve metrics from cache for !module !version !sha1", array('!sha1' => $sha1, '!module' => $module->name,"!version" => $version)), 'debug');
            return $dataCache->data;
        }
    }
    drush_log(dt("NED - Calculate metrics information for !module !version !sha1", array('!sha1' => $sha1, '!module' => $module->name,"!version" => $version)), 'debug');
    $modulesMetrics = array();

    // calculate number of files and lines.
    $filecount = 0;
    $files = glob(dirname($module->filename) . "/*.{php,inc,module}", GLOB_BRACE);
    $fileLineNumber = 0;
    if ($files) {
        $filecount = count($files);
        foreach ($files as $file) {
             $fileLineNumber += count(file($file));
        }
    }
    $modulesMetrics['Number of files'] = $filecount;
    $modulesMetrics['Number of Lines'] = $fileLineNumber;

    // calculate hook stats.
    $hooks = array (
     'hook',
     'hook_form_.*_alter',
     'hook_menu',
     'hook_entity_info',
     'hook_views_',
     'hook_schema',
     'hook_node_info',
    );
    foreach ($hooks as $hook) {
        $output = array();
        $rc = array();
        $command = 'grep -r "\* Implements ' . $hook . '" ' . dirname($module->filename);
        exec($command, $output, $rc);
        if ($rc === 0) {
            $modulesMetrics[$hook] = count($output);
        } elseif ($rc === 1) {
            $modulesMetrics[$hook] = 0;
        } else {
            drush_log(dt("NED - error command '!command' : !error", array('!command' => $command,'!error' => $rc)), 'error');
        }
    }

    if ($cache_enabled) {
        drush_cache_set($cacheID, $modulesMetrics);
    }
    return $modulesMetrics;
}


/**
 * Get list of contrib modules not found in drupal 8.
 */
function nexteuropa_export_data_get_modules_contrib_not_in_D8()
{
    $modules= nexteuropa_export_data_get_modules_by_types();
    $modules_contrib = $modules[CONTRIB];
    foreach ($modules_contrib as $key => $module) {
        $drupal_module = nexteuropa_export_data_get_contrib_module_release_info($key, '8.x');
        if (isset($drupal_module['title']) && !empty($drupal_module['title'])) {
            unset($modules_contrib[$key]);
            drush_log(dt("NED - !module exist in 8.x version", array('!module' =>$module->name)), 'debug');
        } else {
            drush_log(dt("NED - !module not exist in 8.x version", array('!module' =>$module->name)), 'debug');
        }
    }
    return $modules_contrib;
}


/**
 * Get release information from drupal.org
 */
function nexteuropa_export_data_get_contrib_module_release_info($module, $version = '7.x')
{
    $cache_enabled=!drush_get_option('no-cache', false);
    if ($cache_enabled) {
        $cacheID = "ned-module-release-${module}-${version}";
        $dataCache = drush_cache_get($cacheID);
        if (!empty($dataCache)) {
            drush_log(dt("NED - Get release information from cache for !module !version", array('!module' => $module,"!version" => $version)), 'debug');
            return $dataCache->data;
        }
    }
    drush_log(dt("NED - Get release information from drupal.org for !module", array('!module' => $module)), 'debug');
    $url = 'https://updates.drupal.org/release-history/'.urlencode($module).'/'.$version;
    $ch = curl_init();
    $timeout = 5;
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, $timeout);
    $modules_info = _xml2array(simplexml_load_string(curl_exec($ch)));
    $httpcode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    if (empty($modules_info) || $httpcode !== 200 ) {
        throw new \Exception('Could not connect to drupal.org. curl error code: '. $httpcode);
    }
    if ($cache_enabled) {
        drush_cache_set($cacheID, $modules_info);
    }
    return $modules_info;
}

/**
 * XML to array.
 */
function _xml2array( $xmlObject, $out = array () )
{
    foreach ( (array) $xmlObject as $index => $node ) {
        $out[$index] = ( is_object($node) ) ? _xml2array($node) : $node;
    }
    return $out;
}
