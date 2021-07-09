export interface FwUploadStatus {
  action: string;
  level: Level;
  percent: number;
  phase: string;
}

export enum Level {
  Info = "info",
  Warning = "warn",
  Error = "error",
}
