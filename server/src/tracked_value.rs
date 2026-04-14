use serde::{Deserialize, Serialize};

#[derive(Default)]
pub struct TrackedValue<T> {
  changed: bool,
  value: T,
}

impl<T> TrackedValue<T> {
  pub fn new(value: T) -> Self {
    Self {
      changed: true,
      value,
    }
  }

  pub fn set(&mut self, value: T) {
    self.changed = true;
    self.value = value;
  }

  pub fn get_mut(&mut self) -> &mut T {
    self.changed = true;

    &mut self.value
  }

  pub fn get(&self) -> &T {
    &self.value
  }

  pub fn has_changed(&self) -> bool {
    self.changed
  }

  pub fn clear_changed(&mut self) {
    self.changed = false;
  }

  pub fn if_changed(&mut self, func: impl FnOnce(&T)) {
    if self.has_changed() {
      func(self.get());
      self.clear_changed();
    }
  }
}

impl<T: Serialize> Serialize for TrackedValue<T> {
  fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
  where
    S: serde::Serializer,
  {
    self.value.serialize(serializer)
  }
}

impl<'de, T: Deserialize<'de>> Deserialize<'de> for TrackedValue<T> {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: serde::Deserializer<'de>,
  {
    T::deserialize(deserializer).map(TrackedValue::new)
  }
}

impl<T> From<T> for TrackedValue<T> {
  fn from(value: T) -> Self {
    TrackedValue::new(value)
  }
}
