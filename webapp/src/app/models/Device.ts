export interface Device {
  bus: string;
  driver: string;
  id: string;
  vendor: string;
  product: string;
  serial: string;
  usbClasses: string[];
  connected: boolean;
  use: string;
}
