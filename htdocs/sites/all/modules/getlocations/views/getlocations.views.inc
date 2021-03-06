<?php

/**
 * @file
 * @author Bob Hutchinson http://drupal.org/user/52366
 * @copyright GNU GPL
 *
 * Getlocations Map Views support.
 */

/**
 * Implements hook_views_plugins().
 */
function getlocations_views_plugins() {

  return array(
    'module' => 'getlocations',
    'style' => array(
      'getlocations' => array(
        'title' => t('GetLocations'),
        'help' => t('Displays rows as a map.'),
        'handler' => 'getlocations_plugin_style_map',
        'theme' => 'getlocations_view_map',
        'theme path' => GETLOCATIONS_PATH . '/views',
        'uses row plugin' => TRUE,
        'uses grouping' => FALSE,
        'uses options' => TRUE,
        'type' => 'normal',
      ),
    ),
  );
}

/**
 * Preprocess function for theme_getlocations_view_map().
 */
function template_preprocess_getlocations_view_map(&$variables) {
  global $language;

  $locations = $variables['view']->style_plugin->rendered_fields;
  $options = $variables['view']->style_plugin->options;
  $base_field = $variables['view']->style_plugin->view->base_field;

  $latlons = array();
  $minmaxes = array('minlat' => 0, 'minlon' => 0, 'maxlat' => 0, 'maxlon' => 0);
  $ct = 0;
  $entity_type = 'node';
  if ($base_field == 'uid') {
    $entity_type = 'user';
  }
  elseif ($base_field == 'tid') {
    $entity_type = 'taxonomy_term';
  }
  elseif ($base_field == 'cid') {
    $entity_type = 'comment';
  }

  if ($options['custom_content_enable'] and !empty($options['custom_content_source'])) {
    $custom_content_source = $options['custom_content_source'];
  }
  else {
    $custom_content_source = NULL;
  }
  if (count($locations)) {
    // we should loop over them and dump bummers with no lat/lon
    foreach ($locations AS $key => $location) {

      if (! isset($location['type']) && isset($location['machine_name'])) {
        $location['type'] = $location['machine_name'];
      }

      $custom_content = $custom_content_source ? $location[$custom_content_source] : '';
      $lid = 0;
      if (module_exists('getlocations_fields') && isset($location['glid']) && $location['glid'] > 0) {
        $lid = $location['glid'];
      }
      elseif (module_exists('location') && isset($location['lid']) && $location['lid'] > 0) {
        $lid = $location['lid'];
        if (isset($location['type'])) {
          $location['field_name'] = getlocations_get_fieldname($location['type'], $entity_type);
        }
      }
      elseif (module_exists('geofield') && isset($location[$base_field]) && $location[$base_field] > 0 ) {
        $lid = $location[$base_field];
        $location['field_name'] = getlocations_get_fieldname($location['type'], $entity_type);
        $location['latitude'] = $location[$location['field_name']];
        $location['longitude'] = $location[$location['field_name'] . '_1'];
      }
      elseif (module_exists('geolocation') && isset($location[$base_field]) && $location[$base_field] > 0 ) {
        $lid = $location[$base_field];
        $location['field_name'] = getlocations_get_fieldname($location['type'], $entity_type);
        $location['latitude'] = $location[$location['field_name']];
        $location['longitude'] = $location[$location['field_name'] . '_1'];
      }
      if ($lid > 0) {
        if ($entity_type == 'node') {
          $marker = $options['node_map_marker'];
          if (isset($location['type']) && isset($location['field_name']) ) {
            if (isset($options['node_marker__' . $location['type'] . '__' . $location['field_name']])) {
              $marker = $options['node_marker__' . $location['type'] . '__' . $location['field_name']];
            }
          }
          // term marker
          $getlocations_term_marker = variable_get('getlocations_term_marker', array('enable' => 0, 'vids' => 0, 'max_depth' => ''));
          if ($getlocations_term_marker['enable'] && $getlocations_term_marker['vids']) {
            $vids = $getlocations_term_marker['vids'];
            foreach ($vids AS $vid) {
              if ($nid = getlocations_get_nid_from_lid($lid)) {
                $query = db_select('taxonomy_index', 't');
                $query->fields('t', array('tid'));
                $query->join('taxonomy_term_data', 'd', 't.tid=d.tid');
                $query->condition('t.nid', $nid);
                $query->condition('d.vid', $vid);
                $result = $query->execute()->fetchObject();
                if (isset($result->tid) && $result->tid > 0 && isset($options['term_marker_' . $result->tid])) {
                  $marker = $options['term_marker_' . $result->tid];
                }
              }
            }
          }
        }
        elseif ($entity_type == 'taxonomy_term') {
          $marker = $options['vocabulary_map_marker'];
          if (isset($location['field_name'])) {
            if (isset($options['vocabulary_marker_' . $location['field_name']])) {
              $marker = $options['vocabulary_marker_' . $location['field_name']];
            }
          }
        }
        elseif ($entity_type == 'user') {
          $marker = $options['user_map_marker'];
        }
        elseif ($entity_type == 'comment') {
          $marker = $options['comment_map_marker'];
        }
        // per item marker
        if (isset($location['marker']) && ! empty($location['marker'])) {
          $marker = $location['marker'];
        }
        if (getlocations_latlon_check($location['latitude'] . ',' . $location['longitude']) ) {
          $minmaxes = getlocations_do_minmaxes($ct, $location, $minmaxes);
          $ct++;
          $name = htmlspecialchars_decode(isset($location['name']) && $location['name'] ? strip_tags($location['name']) : (isset($location['title']) && $location['title'] ? strip_tags($location['title']) : ''), ENT_QUOTES);

          $cat = '';
          if (isset($location['type'])) {
            $cat = $location['type'];
          }

          $latlons[] = array($location['latitude'], $location['longitude'], $lid, $name, $marker, $base_field, $custom_content, $cat);
        }
      }
    }
  }
  if ($ct < 2 || $lid == 0) {
    unset($minmaxes);
    $minmaxes = '';
  }

  // get the defaults and override with the style plugin options
  $newdefaults = array();
  $getlocations_defaults = getlocations_defaults();
  foreach ($getlocations_defaults AS $k => $v) {
    if (isset($options[$k])) {
      $newdefaults[$k] = $options[$k];
    }
    else {
      $newdefaults[$k] = $getlocations_defaults[$k];
    }
  }

  $mapid = getlocations_setup_map($newdefaults);
  getlocations_js_settings_do($newdefaults, $latlons, $minmaxes, $mapid);

  $variables['map']  = theme('getlocations_show', array('width' => $newdefaults['width'], 'height' => $newdefaults['height'], 'defaults' => $newdefaults, 'mapid' => $mapid, 'type' => '', 'node' => ''));

}
