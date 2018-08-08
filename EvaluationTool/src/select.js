import React from 'react';
import { Label, Dropdown } from 'semantic-ui-react';

const Select = ({ allowNull, value, setValue }) => (
  <Dropdown item text={value || '-'}>
    <Dropdown.Menu>
      {!!allowNull && (
        <Dropdown.Item onClick={() => setValue()} active={!value}>
          <Label circular empty />
          -
        </Dropdown.Item>
      )}
      <Dropdown.Item onClick={() => setValue('mode')} active={value === 'mode'}>
        <Label circular empty />
        mode
      </Dropdown.Item>
      <Dropdown.Item
        onClick={() => setValue('orientation')}
        active={value === 'orientation'}
      >
        <Label circular empty />
        orientation
      </Dropdown.Item>
      <Dropdown.Item onClick={() => setValue('file')} active={value === 'file'}>
        <Label circular empty />
        file
      </Dropdown.Item>
      <Dropdown.Item
        onClick={() => setValue('lambda')}
        active={value === 'lambda'}
      >
        <Label color="blue" circular empty />
        lambda
      </Dropdown.Item>
      <Dropdown.Item
        onClick={() => setValue('error')}
        active={value === 'error'}
      >
        <Label color="red" circular empty />
        error
      </Dropdown.Item>
      <Dropdown.Item
        onClick={() => setValue('steps')}
        active={value === 'steps'}
      >
        <Label color="green" circular empty />
        steps
      </Dropdown.Item>
    </Dropdown.Menu>
  </Dropdown>
);

export default Select;
