<?php

function msm_collection_list() {
  $collections = msm_post('collections', [
    'project' => ['name' => 1, 'database' => 1, 'collection' => 1, 'type' => 1,
      'enabled' => 1, 'source' => 1, 'dependencies' => 1, 'locked' => 1,
    ],
    'sort' => ['weight' => 1, 'name' => 1, '_id' => 1],
  ]);
  dsm($collections);
  $build = array();
  $build['list'] = array(
    '#theme' => 'table',
    '#header' => array(
      t('Name'),
      t('Database'),
      t('Collection'),
      t('Type'),
      t('Status'),
      t('Source Collection'),
      t('Dependencies'),
      t('Actions'),
    ),
    '#rows' => array(),
  );
  foreach ($collections AS $col) {
    $actions = array(
      '#theme' => 'links',
      '#links' => array(),
      '#attributes' => array('class' => array('links', 'inline')),
    );
    if (msm_collection_access($col, 'endpoint')) {
      $actions['#links']['endpoint'] = array(
        'href' => sprintf('admin/msm/col/%s/endpoint', $col['_id']),
        'title' => t('Endpoint'),
      );
    }
    if (msm_collection_access($col, 'settings')) {
      $actions['#links']['settings'] = array(
        'href' => sprintf('admin/msm/col/%s/settings', $col['_id']),
        'title' => t('Settings'),
      );
    }
    if (msm_collection_access($col, 'schema')) {
      $actions['#links']['schema'] = array(
        'href' => sprintf('admin/msm/col/%s/schema', $col['_id']),
        'title' => t('Schema'),
      );
    }
    if (msm_collection_access($col, 'eval')) {
      $actions['#links']['eval'] = array(
        'href' => sprintf('admin/msm/col/%s/custom', $col['_id']),
        'title' => t('Eval'),
      );
    }
    elseif (msm_collection_access($col, 'pipeline')) {
      $actions['#links']['pipeline'] = array(
        'href' => sprintf('admin/msm/col/%s/pipeline', $col['_id']),
        'title' => t('Pipeline'),
      );
    }
    elseif (msm_collection_access($col, 'mapReduce')) {
      $actions['#links']['map-reduce'] = array(
        'href' => sprintf('admin/msm/col/%s/map-reduce', $col['_id']),
        'title' => t('Map Reduce'),
      );
    }
    if (msm_collection_access($col, 'process')) {
      $actions['#links']['process'] = array(
        'href' => sprintf('admin/msm/col/%s/process', $col['_id']),
        'title' => t('Process'),
      );
    }
    if (msm_collection_access($col, 'delete')) {
      $actions['#links']['delete'] = array(
        'href' => sprintf('admin/msm/col/%s/delete', $col['_id']),
        'title' => t('Delete'),
      );
    }
    $dependencies = sprintf('<em>%s</em>', t('None'));
    if (!empty($col['dependencies'])) {
      $dependencies = array(
        '#theme' => 'links',
        '#links' => array(),
      );
      foreach ($col['dependencies'] AS $dep) {
        $dependencies['#links'][] = array(
          'title' => $dep,
          'href' => sprintf('admin/msm/col/%s', $dep),
        );
      }
      $dependencies = render($dependencies);
    }
    $source = !empty($col['source']) ? l($col['source'], sprintf('admin/msm/col/%s', $col['source'])) : 'n/a';
    $build['list']['#rows'][] = array(
      l($col['name'], sprintf('admin/msm/col/%s', $col['_id'])),
      l($col['database'], sprintf('admin/msm/db/%s', $col['database'])),
      $col['collection'],
      empty($col['type']) ? 'n/a' : $col['type'],
      $col['enabled'] ? t('Enabled') : t('Disabled'),
      $source,
      $dependencies,
      render($actions),
    );
  }
  return $build;
}

function msm_collection_view(array $collection, array $options = array()) {
  $options += array(
    'info' => TRUE,
    'schema_info' => TRUE,
    'schema' => TRUE,
    'test_result' => TRUE,
  );
  $module_path = drupal_get_path('module', 'msm');
  $build = array(
    '#theme' => 'msm_collection_view',
  );
  $has_schema = !empty($collection['schema']);
  if ($options['info']) {
    $count = msm_request(sprintf('collection/%s/count', $collection['_id']));
    if ($count['ok']) {
        $count = $count['count'];
    }
    else $count = 'err';

    $build['#info'] = array(
      '#theme' => 'msm_info',
      '#title' => t('Collection details'),
      '#items' => array(
        '_id' => array(t('Collection ID'), $collection['_id']),
        'name' => array(t('Name/Label'), $collection['name']),
        'collection' => array(t('Collection Name'), $collection['collection']),
        'database' => array(t('Database'), l($collection['database'], 'admin/msm/db/' . $collection['database'])),
        'type' => array(t('Collection Type'), isset($collection['type']) ? $collection['type'] : null),
        'schema' => array(t('Has Schema'), $has_schema ? t('Yes') : t('No')),
        'totalDocuments' => array(t('Total Documents'), number_format($count, 0)),
      ),
    );
    if (!empty($collection['source'])) {
      $build['#info']['#items'][] = array(t('Source Collection ID'), l($collection['source'], 'admin/msm/col/' . $collection['source']));
    }
    if (!empty($collection['dependencies'])) {
      $deplinks = [];
      foreach ($collection['dependencies'] AS $dep) {
        $deplinks[] = array(
          'href' => sprintf('admin/msm/col/%s', $dep),
          'title' => $dep,
        );
      }
      $build['#info']['#items']['dependencies'] = array(
        t('Dependencies: (@count)', array('@count' => count($collection['dependencies']))),
        theme('links', array('links' => $deplinks))
      );
    }
  }
  if (!empty($collection['lastProcessed'])) {
    $build['#info']['#items']['lastProcessed'] = array(t('Last processed:'), $collection['lastProcessed']);
    $build['#info']['#items']['timesProcessed'] = array(t('Times processed:'), $collection['timesProcessed']);
    $build['#info']['#items']['millisProcessed'] = array(t('Milliseconds spent processing:'), number_format($collection['millisProcessed']));
    $build['#info']['#items']['averageSeconds'] = array(t('Average processing time:'), number_format($collection['millisProcessed']/$collection['timesProcessed']/1000, 2) . ' seconds');
  }
  if ($options['schema_info'] && $has_schema) {
    $fields = msm_schema_flatten_fields($collection['schema'], MSM_SCHEMA_FLATTEN_ALL);
    $build['#schema_info'] = array(
      '#theme' => 'msm_info',
      '#title' => t('Schema breakdown'),
      '#items' => array(
        'total_fields' => array(t('Total fields'), count($fields)),
      ),
    );
    $unused_fields = array();
    $counts = [];
    foreach ($fields AS $field_name => $field) {
      if (isset($field['percentContaining']) && $field['percentContaining'] <= 0) {
        $unused_fields[] = $field_name;
      }
      $type = $field['type'];
      if (!isset($counts[$type])) $counts[$type] = 0;
      $counts[$type]++;
    }
    foreach ($counts AS $type => $count) {
      $build['#schema_info']['#items'][] = array(ucfirst($type), $count);
    }
    if ($unused_fields) {
      $build['#schema_info']['#items'][] = array(t('Unused fields'), theme('item_list', array('items' => $unused_fields)));
    }
  }
  if ($options['schema']) {
    if (!empty($collection['schema'])) {
      $build['#schema'] = array(
        '#prefix' => sprintf('<h3>%s</h3>', t('Schema')),
        '#theme' => 'msm_schema_table',
        '#schema' => msm_schema_sort($collection['schema']),
        '#flatten' => TRUE,
      );
    }
    else {
      $build['#schema'] = array(
        '#markup' => t('No schema exists yet.'),
      );
    }
  }
  if ($options['test_result']) {
    if ($test = msm_collection_test_load($collection['_id'])) {
      $build['#test_result'] = array(
        '#theme' => 'msm_info',
        '#title' => t('Test Result'),
        '#items' => array(
          'result' => array(t('Result'), $test['pass'] ? t('Pass') : t('FAIL')),
          'last' => array(t('Last test ran on'), $test['last']),
        ),
      );
      if (!$test['pass']) {
        $build['#test_result']['#items']['error'] = array(t('Error'), $test['error']);
      }
    }
  }
  return $build;
}

function msm_collection_schema_view(array $collection) {
  if (!empty($collection['schema'])) {
    $build['schema'] = array(
      '#theme' => 'msm_schema_table',
      '#schema' => $collection['schema'],
      '#flatten' => TRUE,
    );
  }
  else {
    $build['schema'] = array(
      '#markup' => t('No schema exists yet.'),
    );
  }
  return $build;
}

function msm_collection_form($form, &$form_state, array $collection = array()) {
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = sprintf('</div>');
  $form['#is_new'] = empty($collection['_id']);
  $form_state += array(
    'collection_id' => empty($collection['_id']) ? NULL : $collection['_id'],
    'document' => NULL,
  );
  $ajax = array(
    'callback' => 'msm_collection_form_ajax',
    'wrapper' => $html_id,
  );
  $document = !empty($form_state['document']) ? $form_state['document'] : $collection;
  $document += array(
    'name' => null,
    'enabled' => FALSE,
    'persist' => 1,
    'type' => 'custom',
    'source' => null,
    'database' => variable_get('msm_db_default', 'test'),
    'collection' => null,
    'description' => null,
  );
  // "has (collection) source"
  $hasSource = in_array($document['type'], array('mapReduce', 'aggregation', 'custom'));
  $form['collection_id'] = array(
    '#type' => 'item',
    '#title' => t('Collection ID') . ':',
    '#markup' => $form_state['collection_id'],
    '#attributes' => array('class' => array('container-inline')),
    '#access' => !empty($form_state['collection_id']),
  );
  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#weight' => 0,
    '#default_value' => $document['name'],
  );
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#weight' => 5,
    '#default_value' => $document['description'],
  );
  $form['type'] = array(
    '#title' => t('Type'),
    '#type' => 'select',
    '#options' => msm_collection_type_options($document['persist']),
    '#required' => TRUE,
    '#default_value' => $document['type'],
    '#ajax' => $ajax,
    '#weight' => 1000,
  );
  $form['persist'] = array(
    '#title' => t('Storage type'),
    '#type' => 'radios',
    '#options' => array(
      1 => t('Normal (persist)'),
      0 => t('Real-time Query Container (no storage)'),
    ),
    '#default_value' => $document['persist'] ? 1 : 0,
    '#access' => in_array($document['type'], array('aggregation', 'custom')),
    '#ajax' => $ajax,
    '#required' => TRUE,
    '#weight' => 1100,
  );

  $form['database'] = array(
    '#title' => t('Database'),
    '#type' => 'select',
    '#options' => msm_database_options(),
//    '#default_value' => $form['#source']['database'],
    '#disabled' => !$form['#is_new'] || !in_array($document['type'], array('mapReduce', 'custom', 'import')),
    '#default_value' => $document['database'],
    '#required' => TRUE,
    '#weight' => 2000,
  );
  $form['collection'] = array(
    '#title' => t('Collection name'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#weight' => 3000,
    '#default_value' => $document['collection'],
    '#disabled' => !$form['#is_new'],
  );
  $form['source'] = array(
    '#title' => t('Source collection'),
    '#type' => 'select',
    '#options' => array('') + msm_collection_options(NULL, TRUE),
    '#access' => !in_array($document['type'], array('existing', 'import')),
    '#default_value' => $document['source'],
    '#ajax' => $ajax,
    '#required' => TRUE,
    '#weight' => 4000,
  );
  if (!empty($document['source'])) {
    if (empty($form['#source']) || $form['#source']['_id'] != $document['source']) {
      $form['#source'] = msm_request(sprintf('collection/%s', $document['source']));
    }
  }

  switch ($document['type']) {
    case 'existing':
      module_load_include('inc', 'msm', 'includes/database');
      $database = msm_database_load($document['database']);
      $form['collection']['#type'] = 'select';
      $form['collection']['#options'] = msm_database_uninitialized_collection_options($database);
      break;
    case 'custom':
      break;
    case 'mapReduce':
      break;
  }

  $form['actions'] = array('#type' => 'actions', '#weight' => 10000);
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  return $form;
}

function msm_collection_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_form_validate(&$form, &$form_state) {
  $values = $form_state['values'];
  $document =& $form_state['document'];
  $skip = array('form_id', 'form_build_id', 'form_token', 'op', 'submit', 'destination');
  foreach (array_keys($values) AS $key) {
    if (in_array($key, $skip)) continue;
    $document[$key] = $values[$key];
  }
  $document['persist'] = intval($document['persist']) ? true : false;
  if (empty($document['source'])) unset($document['source']);
  if ($form['#is_new'] && empty($document['_id']) && !empty($document['database']) && !empty($document['collection'])) {
    $collection_id = sprintf('%s:%s', $document['database'], $document['collection']);
    if (!$result = msm_post('collections', array('limit' => 1, 'query' => array('_id' => $collection_id)))) {
      $form_state['collection_id'] = $collection_id;
    }
    else {
      $parents = implode('][', $form['collection']['#parents']);
      form_set_error($parents, t('Collection ID (@id) is already in use.', array('@id' => $collection_id)));
    }
  }
}

function msm_collection_form_submit($form, &$form_state) {
  $document = $form_state['document'];
  if ($form['#is_new']) {
    $result = msm_post(sprintf('collection/create/%s', $form_state['collection_id']), $document);
  }
  else {
    $result = msm_post(sprintf('collection/%s/update', $form_state['collection_id']), ['$set' => $document]);
  }
  if (!isset($result['ok']) || !$result['ok']) {
    $result = (array)$result;
    $result += array('error' => t('No further information to display.'));
    form_set_error('', t('An error occurred while attempting to save the collection: @err.', array('@err' => $result['error'])));
  }
  $form_state['redirect'] = sprintf('admin/msm/col/%s', $form_state['collection_id']);
}

function msm_collection_clone_form($form, &$form_state, array $collection) {
  $form_state['original_collection_id'] = $collection['_id'];
  $cloned = $collection;
  unset($cloned['_id']);
  unset($cloned['collection']);
  $cloned['name'] = sprintf('Clone of %s', $collection['name']);
  $values = empty($form_state['clone']) ? array() : $form_state['clone'];
  $form['clone'] = msm_collection_form($form, $values, $cloned);
  unset($form['clone']['actions']);
  $form['clone'] += array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
  );
  $form['options'] = array('#tree' => TRUE);
  $form['options']['delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete this collection after cloning.'),
  );
  $form['options']['process'] = array(
    '#type' => 'checkbox',
    '#title' => t('Trigger processing on the newly cloned collection.'),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Clone collection'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/msm/col/' . $collection['_id'],
    '#title' => t('Cancel'),
  );
  return $form;
}

function msm_collection_clone_form_validate(&$form, &$form_state) {
  $values = array('values' => $form_state['values']['clone']);
  msm_collection_form_validate($form['clone'], $values);
  $form_state['clone'] = $values;
}


function msm_collection_clone_form_submit(&$form, &$form_state) {
  if (!$data = msm_request(sprintf('collection/%s', $form_state['original_collection_id']))) {
    form_set_error('', t('An error occurred.'));
  }
  unset($data['_id']);
  $data = array_merge($data, $form_state['clone']['document']);
  $result = msm_post(sprintf('collection/create/%s', $form_state['clone']['collection_id']), $data);
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/msm/col/%s', $form_state['clone']['collection_id']);
  }
  else form_set_error('', t('An error has occurred: @error', array('@error' => $result['error'])));

  $options = $form_state['values']['options'];
  if ($options['delete']) {
    msm_request(sprintf('collection/%s/delete', $form_state['original_collection_id']));
  }
  if ($options['process']) {
    msm_request(sprintf('collection/%s/process', $form_state['clone']['collection_id']));
  }
}

function msm_collection_create_form($form, &$form_state) {
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = sprintf('</div>');
  $ajax = array(
    'callback' => 'msm_collection_create_form_ajax',
    'wrapper' => $html_id,
  );
  $document = !empty($form_state['document']) ? $form_state['document'] : array();
  $document += array(
    'enabled' => FALSE,
    'persist' => null,
    'type' => null,
    'source' => null,
  );
  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#access' => !empty($document['source']) || $document['type'] == 'import',
    '#required' => TRUE,
    '#weight' => 95,
    '#ajax' => $ajax + array('event' => 'blur'),
  );
  $form['machine'] = array(
    '#title' => t('Machine'),
    '#type' => 'machine_name',
    '#machine_name' => array(
      'exists' => 'msm_collection_exists',
    ),
    '#access' => !empty($document['source']) || $document['type'] == 'import',
    '#required' => TRUE,
    '#weight' => 97,
  );
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create collection'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/msm/col',
    '#title' => t('Cancel'),
  );
  $form['persist'] = array(
    '#title' => t('Storage type'),
    '#type' => 'radios',
    '#options' => array(
      1 => t('Normal (persist)'),
      0 => t('Real-time Query Container (no storage)'),
    ),
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  $form['type'] = array(
    '#title' => t('Collection type'),
    '#type' => 'select',
    '#options' => msm_collection_type_options(),
    '#access' => $document['persist'] !== null,
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  $form['source'] = array(
    '#title' => t('Source collection'),
    '#type' => 'select',
    '#options' => array('') + msm_collection_options(),
    '#access' => !empty($document['type']) && $document['type'] != 'import',
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  if (!empty($document['source'])) {
    if (empty($form['#source']) || $form['#source']['_id'] != $document['source']) {
      $form['#source'] = msm_request(sprintf('collection/%s', $document['source']));
    }
  }
  if ($document['persist'] && !empty($document['source'])) {
    $document += array('destination' => array());
    $document['destination'] += array(
      'database' => $form['#source']['database'],
      'collection' => NULL,
    );
    $form['destination'] = array(
      '#tree' => TRUE,
      '#access' => !empty($document['source']) && $document['persist'],
    );
    if ($document['type'] == 'mapReduce') {
      $form['destination']['database'] = array(
        '#title' => t('Destination database'),
        '#type' => 'select',
        '#options' => msm_database_options(),
        '#default_value' => $form['#source']['database'],
        '#required' => TRUE,
      );
    }
    else {
        $form['destination']['database'] = array(
          '#type' => 'value',
          '#value' => $document['destination']['database'],
        );
    }
    $form['destination']['collection'] = array(
      '#title' => t('Destination collection'),
      '#type' => 'textfield',
      '#description' => t('Specify the name of the collection to create.'),
      '#required' => TRUE,
      '#ajax' => $ajax += array('event' => 'blur'),
    );
  }
  if (empty($form_state['id'])) {
    $form['actions']['submit']['#access'] = FALSE;
  }

  $form_state['document'] = $document;
  return $form;
}

function msm_collection_create_form_validate(&$form, &$form_state) {
  $values = $form_state['values'];
  $document =& $form_state['document'];
  $skip = array('form_id', 'form_build_id', 'form_token', 'machine', 'op', 'submit', 'destination');
  foreach (array_keys($values) AS $key) {
    if (in_array($key, $skip)) continue;
    $document[$key] = $values[$key];
  }
  $document['persist'] = intval($document['persist']) ? true : false;
  if (!$document['persist']) {
    $form_state['id'] = empty($values['machine']) ? null : $values['machine'];
    $document['collection'] = $form['#source']['collection'];
    $document['database'] = $form['#source']['database'];
  }
  else {
    if ($document['type'] == 'import') {

    }
    elseif (!empty($values['destination']['collection']) && !empty($values['destination']['database'])) {
      $document['collection'] = $values['destination']['collection'];
      $document['database'] = $values['destination']['database'];
      $form_state['id'] = sprintf('%s.%s', $document['database'], $document['collection']);
    }
    else $form_state['id'] = null;
  }
}

function msm_collection_create_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_create_form_submit($form, &$form_state) {
  $result = msm_post(sprintf('collection/create/%s', $form_state['id']), $form_state['document']);
  if ($result['ok']) {
    switch ($form_state['document']['type']) {
      case 'mapReduce';
        $step = 'map-reduce';
        break;
      case 'aggregation';
        $step = 'pipeline';
        break;
      case 'custom':
        $step = 'custom';
        break;
    }
    $form_state['redirect'] = sprintf('admin/msm/col/%s/%s', $form_state['id'], $step);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_delete_form($form, &$form_state, $collection) {
  $form['#collection'] = $collection;
  if ($result = msm_request(sprintf('collection/%s/dependents', $collection['_id']))) {
    $form['dependents'] = array(
      '#type' => 'markup',
      '#markup' => theme('item_list', array(
        'items' => $result,
        'title' => t('Collections dependending on @name:', array('@name' => $collection['name'])),
      )),
    );
  }
  return confirm_form($form,
    t('Are you sure you want to delete all schema information for @id', array('@id' => $collection['_id'])),
    sprintf('admin/msm/col/%s', $collection['_id']),
    t('No user database will be deleted. Only schema information.'),
    t('Delete collection'),
    t('Cancel')
  );
}

function msm_collection_delete_form_submit($form, &$form_state) {
  $result = msm_request(sprintf('collection/%s/delete', $form['#collection']['_id']));
  if ($result['ok']) {
    $form_state['redirect'] = 'admin/msm/col';
  }
}

function msm_collection_type_options($persistant = TRUE) {
  if ($persistant) {
    return array(
      'mapReduce' => t('Map Reduce'),
      'aggregation' => t('Aggregation'),
      'custom' => t('Custom'),
      'import' => t('Import'),
      'existing' => t('Existing'),
    );
  }
  return array(
    'aggregation' => t('Aggregation'),
    'custom' => t('Custom'),
  );
}

function msm_collection_exists() {
  $args = func_get_args();
  return false;
}

function msm_collection_settings_form($form, &$form_state, array $collection) {
  $collection = json_decode(json_encode($collection), TRUE);
  $collection += array(
    'preProcess' => NULL,
    'postProcess' => NULL,
    'preExecute' => NULL,
    'postExecute' => NULL,
    'type' => 'existing',
    'persist' => FALSE,
    'weight' => 0,
  );
  $weight_options = range(-300, 300);
  $form['weight'] = array(
      '#type' => 'select',
      '#title' => t('Weight'),
      '#required' => TRUE,
      '#options' => array_combine($weight_options, $weight_options),
      '#default_value' => $collection['weight'],
  );
  $form['process'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Processing'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($collection['preProcess']) && empty($collection['postProcess']),
    '#access' => $collection['persist'],
  );
  $form['process']['preProcess'] = array(
    '#title' => t('Pre-process code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#default_value' => $collection['preProcess'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['process']['postProcess'] = array(
    '#title' => t('Post-process code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['postProcess'],
  );
  $form['execute'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Execution'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($collection['preExecute']) && empty($collection['postExecute']),
  );
  $form['execute']['preExecute'] = array(
    '#title' => t('Pre-execute code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['preExecute'],
  );
  $form['execute']['postExecute'] = array(
    '#title' => t('Post-execute code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['postExecute'],
  );
  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#default_value' => $collection['enabled'],
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['#collection'] = $collection;
  return $form;
}

function msm_collection_settings_form_validate($form, &$form_state) {
}

function msm_collection_settings_form_submit($form, &$form_state) {
  $fields = array('preExecute', 'postExecute', 'preProcess', 'postProcess');
  $values = $form_state['values'];
  $update = array(
    '$set' => array(
      'enabled' => (bool)$values['enabled'],
      'weight' => (int)$values['weight'],
    ),
    '$unset' => array(),
  );
  $collectionId = $form['#collection']['_id'];
  foreach ($fields AS $f) {
    if (empty($values[$f])) {
      $update['$unset'][$f] = "";
    }
    else $update['$set'][$f] = $values[$f];
  }
  if (empty($update['$unset'])) {
    unset($update['$unset']);
  }
  $result = msm_post(sprintf('collection/%s/update', $collectionId), $update);
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/msm/col/%s/view', $collectionId);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_endpoint(array $collection) {
  return drupal_get_form('msm_collection_endpoint_form', $collection);
}

function msm_collection_endpoint_form($form, &$form_state, array $collection) {
  $params = empty($form_state['values']['params']) ? array() : $form_state['values']['params'];
  $params += array(
    'limit' => 1,
    'skip' => 0,
    'query' => null,
    'project' => null,
    'sort' => null,
    'options' => array(),
  );
  $params['options'] += array(
    'array_item_limit' => 3,
  );
  $result = msm_post(sprintf('collection/%s/query', $collection['_id']), $params);
  dsm($result);
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';
  $ajax = array(
    'callback' => 'msm_collection_endpoint_form_ajax',
    'wrapper' => $html_id,
  );
  $form['params'] = array(
    '#type' => 'fieldset',
    '#title' => t('Parameters'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form['params']['query'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Query'),
    '#default_value' => $params['query'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['sort'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Sort'),
    '#default_value' => $params['sort'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['project'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Project'),
    '#default_value' => $params['project'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['skip'] = array(
    '#type' => 'textfield',
    '#title' => t('Skip'),
    '#default_value' => $params['skip'],
  );
  $form['params']['update'] = array(
    '#type' => 'button',
    '#value' => t('Run query'),
    '#ajax' => $ajax,
  );
  $form['params']['options'] = array('#tree' => TRUE);
  $form['params']['options']['array_item_limit'] = array(
    '#title' => t('Array Item Limit'),
    '#type' => 'textfield',
    '#default_value' => $params['options']['array_item_limit'],
  );
dsm($params['options']);
  if (!empty($result['results'][0])) {
    $form['result'] = array(
      '#markup' => theme('msm_schema_result_tree', array(
        'schema' => $result['schema'],
        'result' => $result['results'][0],
        'array_item_limit' => (int)$params['options']['array_item_limit'],
      )),
    );
  }
  else {
    $form['empty'] = array(
      '#markup' => sprintf('<p>%s</p>', t('No results were found that matched your search.')),
    );
  }
  $form['total'] = array(
    '#type' => 'item',
    '#title' => t('Total documents:'),
    '#markup' => $result['totalResults'],
  );
  $form['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('clearfix')),
  );
  if (((int)$params['skip']) < ($result['totalResults']-1)) {
    $form['actions']['next'] = array(
      '#type' => 'button',
      '#value' => t('Next'),
      '#attributes' => array('style' => 'float:right;'),
      '#ajax' => $ajax,
    );
  }
  if ($params['skip']) {
    $form['actions']['prev'] = array(
      '#type' => 'button',
      '#value' => t('Previous'),
      '#ajax' => $ajax,
    );
  }
  return $form;
}

function msm_collection_endpoint_form_validate(&$form, &$form_state) {
  $params = $form_state['values']['params'];
  $input = &$form_state['input']['params'];

  $ele = $form_state['triggering_element'];
  if ($ele['#parents'][0] == 'next') {
    $params['skip'] = ((int)$params['skip'])+1;
    $input['skip'] = $params['skip'];
  }
  elseif ($ele['#parents'][0] == 'prev') {
    $params['skip'] = ((int)$params['skip'])-1;
    $input['skip'] = $params['skip'];
  }
  $result = array();
  foreach ($params AS $k => $v) {
    if ($v && !empty($v) && $k != 'update') {
      $result[$k] = $v;
    }
  }
  foreach (array('project', 'sort', 'query') AS $k) {
    if (!empty($result[$k])) {
      if (!$result[$k] = json_decode($result[$k])) {
        form_set_error("params][$k", t('Invalid JSON detected.'));
      }
    }
  }
  $form_state['values']['params'] = $result;
}

function msm_collection_endpoint_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_process_form($form, &$form_state, array $collection) {
  return $form;
}

function msm_collection_reset_form($form, &$form_state, array $collection) {
  $form['#collection'] = $collection;
  return confirm_form($form,
    t('Are you sure you want to reset the schema for @collection', array('@collection' => $collection['_id'])),
    sprintf('admin/msm/collection/%s', $collection['_id']),
    t('This will remove the schema information.'),
    t('Reset schema'),
    t('Cancel')
  );
}

function msm_collection_reset_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $update = array(
    '$set' => array('enabled' => false),
    '$unset' => array('schema' => '', 'schemaImported' => '', 'analyzingSchema' => ''),
  );
  $result = msm_post(sprintf('collection/%s/update', $collection['_id']), $update);
  if ($result['ok']) {
    drupal_set_message(t('The schema is being reset. Refresh the page momentarily as necessary.'));
    $form_state['redirect'] = sprintf('admin/msm/col/%s/schema/analyze', $collection['_id']);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_schema_analyze_form($form, &$form_state, array $collection) {
  $result = msm_request(sprintf('collection/%s/count', $collection['_id']));
  $form['#attributes'] = array('class' => array('clearfix'));
  if (!$result['ok']) {
    $form['error'] = array(
      '#markup' => t('Failed to retrieve record count from @db -> @collection', array(
        '@db' => $form['#db'],
        '@col' => $form['#cname'],
      )),
    );
  }
  $form['totalRecords'] = array(
    '#type' => 'item',
    '#title' => t('Total records'),
    '#markup' => $result['count'],
  );
  $form['limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Limit'),
    '#description' => t('Limit schema analysis to # records.'),
    '#default_value' => $result['count'],
  );
  drupal_set_title(t('Analyze and import schema for @collection', array('@collection' => $collection['_id'])));
  $form['actions'] = array(
    '#prefix' => sprintf('<p>%s</p>', t('This will analyze the existing data to derrive the schema structure and import it as an "existing" collection.')),
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Analyze & Import Schema'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => sprintf('admin/msm/col/%s', $collection['_id']),
  );
  $form['#collection'] = $collection;
  return $form;
}

function msm_collection_schema_analyze_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $options = array('limit' => $form_state['values']['limit']);
  $result = msm_post(sprintf('collection/%s/analyze', $collection['_id']), $options);
  $form_state['redirect'] = sprintf('admin/msm/col/%s/schema/view', $collection['_id']);
  drupal_set_message(t('The schema is being analyzed. Refresh the page momentarily as necessary.'));
}

function msm_collection_tests_form($form, &$form_state, $collection) {
  $form['#collection_id'] = $collection['_id'];
  if (empty($form_state['values']['test'])) {
    try {
      $test = msm_request(sprintf('test/%s', $collection['_id']));
    }
    catch (Exception $e) {
      $test = array();
    }
  }
  else {
    $test = $form_state['values']['test'];
  }
  $test += array(
    'collection' => $collection['collection'],
    'database' => $collection['database'],
    'units' => array(),
  );
  $test['units'] += array(0 => array());
  $test['units'][0] += array(
    'code' => "let count = await col.count();\nassert(count, 'Collection is empty');",
  );

  if ($test['last']) {
    $form['result'] = array(
      '#type' => 'markup',
      '#markup' => theme('table', array(
        'header' => array(
          t('Result'),
          t('Error'),
          t('Last test ran'),
        ),
        'rows' => array(array(
          $test['pass'] ? t('Pass') : t('FAIL'),
          empty($test['error']) ? '-' : $test['error'],
          $test['last'],
        )),
      )),
    );
  }

  $form['test'] = array('#tree' => TRUE);
  $form['test']['_id'] = array(
    '#type' => 'value',
    '#value' => $collection['_id'],
  );
  $form['test']['collection'] = array(
    '#type' => 'value',
    '#value' => $test['collection'],
  );
  $form['test']['database'] = array(
    '#type' => 'value',
    '#value' => $test['database'],
  );
  $form['test']['code'] = array(
    '#title' => t('Code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#required' => TRUE,
    '#rows' => 20,
    '#default_value' => $test['units'][0]['code'],
  );
  $form['options'] = array('#tree' => TRUE);
  $form['options']['run_test'] = array(
    '#type' => 'checkbox',
    '#title' => t('Run test after saving.'),
    '#default_value' => 1,
  );
  $form['options']['lock'] = array(
    '#type' => 'checkbox',
    '#title' => t('Lock collection after passing test.'),
    '#default_value' => !empty($collection['locked']),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => sprintf('admin/msm/col/%s', $collection['_id']),
    '#title' => t('Cancel'),
  );
  return $form;
}

function msm_collection_tests_form_submit($form, &$form_state) {
  $test = $form_state['values']['test'];
  $test['units'] = array(array(
    'description' => 'Collection Base Tests',
    'code' => $test['code'],
  ));
  unset($test['code']);
  $result = msm_post(sprintf('test/%s', $test['_id']), $test);
  if ($result['ok']) {
    $options = $form_state['values']['options'];
    if ($options['run_test']) {
      $testresult = msm_request(sprintf('test/%s/run', $test['_id']));
      if ($testresult['result']['pass'] && $options['lock']) {
        $res2 = msm_post(sprintf('collection/%s/update', $form['#collection_id']), [
          '$set' => ['locked' => TRUE]
        ]);
        if (!$res2['ok']) {
          form_set_error('', t('An error has occured: @error', array('@error' => $res2['error'])));
        }
      }
    }
  }
}

function msm_collection_test_load($collection_id) {
  try {
    return msm_request(sprintf('test/%s', $collection_id));
  }
  catch (Exception $e) {
    return null;
  }
}
